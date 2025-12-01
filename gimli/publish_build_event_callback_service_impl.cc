#include "gimli/publish_build_event_callback_service_impl.h"

#include <fstream>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "absl/base/nullability.h"
#include "absl/cleanup/cleanup.h"
#include "absl/log/log.h"
#include "absl/strings/match.h"
#include "absl/strings/strip.h"
#include "gimli/recording.pb.h"
#include "gimli/report.h"
#include "gimli/reporter.h"
#include "google/devtools/build/v1/build_events.pb.h"
#include "google/devtools/build/v1/publish_build_event.pb.h"
#include "google/protobuf/any.pb.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/text_format.h"
#include "google/protobuf/util/time_util.h"
#include "grpcpp/server_context.h"
#include "grpcpp/support/server_callback.h"
#include "grpcpp/support/status.h"
#include "src/main/java/com/google/devtools/build/lib/buildeventstream/proto/build_event_stream.pb.h"

namespace gimli {
namespace {
using ::build_event_stream::BuildEvent;
using ::build_event_stream::BuildEventId;
using ::google::devtools::build::v1::PublishBuildToolEventStreamRequest;
using ::google::devtools::build::v1::PublishBuildToolEventStreamResponse;
using ::google::devtools::build::v1::PublishLifecycleEventRequest;
using ::google::protobuf::util::TimeUtil;

std::string_view PayloadName(BuildEvent::PayloadCase payload) {
  const auto* message_descriptor = BuildEvent::descriptor();
  const auto* field_descriptor = message_descriptor->FindFieldByNumber(payload);
  return (field_descriptor == nullptr) ? "Unknown" : field_descriptor->name();
}

std::string_view IdName(BuildEventId::IdCase id) {
  const auto* message_descriptor = BuildEventId::descriptor();
  const auto* field_descriptor = message_descriptor->FindFieldByNumber(id);
  return (field_descriptor == nullptr) ? "Unknown" : field_descriptor->name();
}

}  // namespace

PublishBuildEventCallbackServiceImpl::PublishBuildEventCallbackServiceImpl(
  Reporter& reporter, std::optional<std::filesystem::path> testdata)
  : reporter_(&reporter), testdata_(std::move(testdata)) {}

grpc::ServerUnaryReactor*
PublishBuildEventCallbackServiceImpl::PublishLifecycleEvent(
  grpc::CallbackServerContext* context,
  const PublishLifecycleEventRequest* request,
  ::google::protobuf::Empty* response) {
  // TODO: use the life cycle event to clear & publish the results of
  // a build for a given workspace directory.
  auto* reactor = context->DefaultReactor();
  reactor->Finish(grpc::Status::OK);
  return reactor;
}

grpc::ServerBidiReactor<PublishBuildToolEventStreamRequest,
                        PublishBuildToolEventStreamResponse>*
PublishBuildEventCallbackServiceImpl::PublishBuildToolEventStream(
  grpc::CallbackServerContext* context) {
  class Reactor final
    : public grpc::ServerBidiReactor<PublishBuildToolEventStreamRequest,
                                     PublishBuildToolEventStreamResponse> {
   public:
    Reactor(Reporter* absl_nonnull reporter,
            const StderrProcessor* absl_nonnull stderr_processor,
            std::optional<std::filesystem::path> testdata)
      : reporter_(reporter),
        stderr_processor_(stderr_processor),
        testdata_(std::move(testdata)) {
      StartRead(&request_);
    }

    void OnReadDone(bool ok) final {
      if (testdata_.has_value()) {
        *recording_.add_requests() = request_;
      }
      // The protocol (not very well documented) seems to be that the service
      // must respond with the "identifiers" (stream id and sequence numnber)
      // of the request, so the caller knows they have been acknowledged.
      //
      // The request will be processed once this response is written, because
      // the request's contents will drive the termination or continuation of
      // the streaming RPC call.
      *response_.mutable_stream_id() =
        request_.ordered_build_event().stream_id();
      response_.set_sequence_number(
        request_.ordered_build_event().sequence_number());
      StartWrite(&response_);
    }

    void OnWriteDone(bool ok) final {
      const auto& build_event = request_.ordered_build_event().event();
      if (build_event.has_component_stream_finished()) {
        Finish(grpc::Status::OK);
        return;
      }
      Process(build_event.bazel_event());
      StartRead(&request_);
    }

    void OnDone() final {
      auto _ = absl::MakeCleanup([&]() { delete this; });
      if (report_.has_value()) {
        reporter_->AddReport(*std::move(report_));
      }

      if (!testdata_.has_value()) return;
      if (auto size = labels_.size(); size != 1) {
        LOG(ERROR) << "⏺️ Recording only works for 1 target, got " << size;
        return;
      }
      std::string_view label = labels_.front();
      static constexpr std::string_view kPackage = "//gimli/testdata:";
      if (!absl::StartsWith(label, kPackage)) {
        LOG(ERROR) << "⏺️ Recording only works for target in " << kPackage
                   << ", got " << label;
        return;
      }
      const auto path = (*testdata_ / absl::StripPrefix(label, kPackage))
                          .replace_extension(".textproto");

      std::string contents;
      if (!google::protobuf::TextFormat::PrintToString(recording_, &contents)) {
        LOG(ERROR) << "⏺️ Recording failed, couldn't print to text format";
        return;
      }

      std::fstream stream(path, std::ios::out | std::ios::trunc);
      stream << contents;
      LOG(INFO) << "⏺️ Recorded " << path;
    }

   private:
    void Process(const google::protobuf::Any& bazel_event) {
      BuildEvent build_event;
      if (!bazel_event.UnpackTo(&build_event)) return;
      // Log the events if vlog is enabled via `--vmodule=gimli_server=1`.
      // Mostly seful for learning the poorly documented Build Event Protocol.
      VLOG(1) << "🐱" << IdName(build_event.id().id_case()) << "/"
              << PayloadName(build_event.payload_case()) << " -> "
              << build_event.children_size();
      for (const auto& child : build_event.children()) {
        VLOG(1) << "  🐶" << IdName(child.id_case());
      }
      // If in recording mode, save the build event and the configured targets
      if (testdata_.has_value()) {
        *recording_.add_build_events() = build_event;
        if (build_event.payload_case() == BuildEvent::kConfigured) {
          labels_.push_back(build_event.id().target_configured().label());
        }
      }

      if (build_event.payload_case() == BuildEvent::kStarted) {
        report_ = Report{
          .workspace_path = build_event.started().workspace_directory(),
          // The precision of timestamp is in nanoseconds so we use that
          // to convert from protobuf timestamp to absl::Time.
          .time = absl::FromUnixNanos(TimeUtil::TimestampToNanoseconds(
            build_event.started().start_time())),
        };
        VLOG(1) << " 🔨 in " << build_event.started().workspace_directory();
      }
      if (build_event.payload_case() == BuildEvent::kProgress) {
        if (report_.has_value()) {
          for (auto&& error :
               stderr_processor_->ToErrors(build_event.progress().stderr())) {
            report_->errors.push_back(std::move(error));
          }
        }
      }
    }

    Reporter* absl_nonnull reporter_;
    const StderrProcessor* absl_nonnull stderr_processor_;
    std::optional<std::filesystem::path> testdata_;
    std::vector<std::string> labels_ = {};
    gimli::Recording recording_ = {};
    std::optional<Report> report_;

    PublishBuildToolEventStreamRequest request_;
    PublishBuildToolEventStreamResponse response_;
  };

  return new Reactor(reporter_, &stderr_processor_, testdata_);
}

}  // namespace gimli

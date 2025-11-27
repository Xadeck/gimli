#include "absl/cleanup/cleanup.h"
#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/log/globals.h"
#include "absl/log/initialize.h"
#include "absl/log/log.h"
#include "absl/strings/match.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "absl/strings/strip.h"
#include "gimli/gimli.grpc.pb.h"
#include "gimli/gimli.pb.h"
#include "google/devtools/build/v1/build_events.pb.h"
#include "google/devtools/build/v1/publish_build_event.grpc.pb.h"
#include "google/devtools/build/v1/publish_build_event.pb.h"
#include "google/protobuf/text_format.h"
#include "grpcpp/ext/proto_server_reflection_plugin.h"
#include "grpcpp/grpcpp.h"
#include "src/main/java/com/google/devtools/build/lib/buildeventstream/proto/build_event_stream.pb.h"
#include <csignal>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <thread>

ABSL_FLAG(uint16_t, port, 8080, "The port where to listen");
ABSL_FLAG(bool, record, false,
          "If true, even stream are recorded in testdata.");

namespace google::devtools::build::v1 {
namespace {
using build_event_stream::BuildEvent;
using build_event_stream::BuildEventId;

absl::string_view PayloadName(BuildEvent::PayloadCase payload) {
  const auto *message_descriptor = BuildEvent::descriptor();
  const auto *field_descriptor = message_descriptor->FindFieldByNumber(payload);
  return (field_descriptor == nullptr) ? "Unknown" : field_descriptor->name();
}

absl::string_view IdName(BuildEventId::IdCase id) {
  const auto *message_descriptor = BuildEventId::descriptor();
  const auto *field_descriptor = message_descriptor->FindFieldByNumber(id);
  return (field_descriptor == nullptr) ? "Unknown" : field_descriptor->name();
}

// TODO: move to a separate library.
class PublishBuildEventCallbackServiceImpl final
    : public PublishBuildEvent::CallbackService {
public:
  PublishBuildEventCallbackServiceImpl(
      std::optional<std::filesystem::path> testdata)
      : testdata_(testdata) {
    //
  }

  grpc::ServerUnaryReactor *
  PublishLifecycleEvent(grpc::CallbackServerContext *context,
                        const PublishLifecycleEventRequest *request,
                        ::google::protobuf::Empty *response) final {
    // TODO: use the life cycle event to clear & publish the results of
    // a build for a given workspace directory.
    auto *reactor = context->DefaultReactor();
    reactor->Finish(grpc::Status::OK);
    return reactor;
  }

  grpc::ServerBidiReactor<PublishBuildToolEventStreamRequest,
                          PublishBuildToolEventStreamResponse> *
  PublishBuildToolEventStream(grpc::CallbackServerContext *context) final {
    class Reactor final
        : public grpc::ServerBidiReactor<PublishBuildToolEventStreamRequest,
                                         PublishBuildToolEventStreamResponse> {
    public:
      Reactor(std::optional<std::filesystem::path> testdata)
          : testdata_(testdata) {
        StartRead(&request_);
      }

      void OnReadDone(bool ok) final {
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
        const auto &build_event = request_.ordered_build_event().event();
        if (build_event.has_component_stream_finished()) {
          Finish(grpc::Status::OK);
          return;
        }
        Process(build_event.bazel_event());
        StartRead(&request_);
      }

      void OnDone() final {
        auto _ = absl::MakeCleanup([&]() { delete this; });

        if (!testdata_.has_value()) {
          return;
        }
        if (auto size = labels_.size(); size != 1) {
          LOG(ERROR) << "⏺️ Recording only works for 1 target, got " << size;
          return;
        }
        absl::string_view label = labels_.front();
        static constexpr absl::string_view kPackage = "//gimli/testdata:";
        if (!absl::StartsWith(label, kPackage)) {
          LOG(ERROR) << "⏺️ Recording only works for target in " << kPackage
                     << ", got " << label;
          return;
        }
        const auto path = (*testdata_ / absl::StripPrefix(label, kPackage))
                              .replace_extension(".textproto");

        std::string contents;
        if (!google::protobuf::TextFormat::PrintToString(recording_,
                                                         &contents)) {
          LOG(ERROR) << "⏺️ Recording failed, couldn't print to text format";
          return;
        }

        std::fstream stream(path, std::ios::out | std::ios::trunc);
        stream << contents;
        LOG(INFO) << "⏺️ Recorded " << path;
      }

    private:
      void Process(const google::protobuf::Any &bazel_event) {
        BuildEvent build_event;
        if (!bazel_event.UnpackTo(&build_event)) {
          return;
        }
        // Log the events if vlog is enabled via `--vmodule=gimli_server=1`.
        // Mostly seful for learning the poorly documented Build Event Protocol.
        VLOG(1) << "🐱" << IdName(build_event.id().id_case()) << "/"
                << PayloadName(build_event.payload_case()) << " -> "
                << build_event.children_size();
        for (const auto &child : build_event.children()) {
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
          // TODO: save the workspace directory to associate errors to it.
          VLOG(1) << " 🔨 in " << build_event.started().workspace_directory();
        }
        if (build_event.payload_case() == BuildEvent::kProgress) {
          // TODO(xdecoret): parse be.progress().stderr() into warnings,
          // errors, and ignoring things in bazel_out.
        }
      }

      std::optional<std::filesystem::path> testdata_;
      std::vector<std::string> labels_;
      gimli::Recording recording_;

      PublishBuildToolEventStreamRequest request_;
      PublishBuildToolEventStreamResponse response_;
    };

    return new Reactor(testdata_);
  }

private:
  std::optional<std::filesystem::path> testdata_;
};

} // namespace
} // namespace google::devtools::build::v1

namespace gimli {
namespace {

class GimliServiceImpl final : public Gimli::Service {
public:
  grpc::Status Notify(grpc::ServerContext *context,
                      const NotifyRequest *request,
                      NotifyResponse *response) final {
    response->set_output("PAPAGEI " + request->input());
    return grpc::Status::OK;
  }
};

} // namespace
} // namespace gimli

using gimli::GimliServiceImpl;
using google::devtools::build::v1::PublishBuildEventCallbackServiceImpl;

namespace {
volatile std::sig_atomic_t interrupted = 0;

void sigint_handler(int signal) {
  interrupted = 1;
  std::signal(signal, SIG_DFL);
}
} // namespace

int main(int argc, char **argv) {

  absl::ParseCommandLine(argc, argv);
  absl::InitializeLog();
  absl::SetStderrThreshold(absl::LogSeverityAtLeast::kInfo);

  std::optional<std::filesystem::path> testdata;
  if (absl::GetFlag(FLAGS_record)) {
    const char *workspace = std::getenv("BUILD_WORKSPACE_DIRECTORY");
    if (workspace == nullptr) {
      std::cerr << "--record only work when executed via `blaze run`\n";
      return 1;
    }
    testdata = std::filesystem::path(workspace) / "gimli" / "testdata";
    LOG(INFO) << "⏺️ Recording in " << *testdata;
  }

  const uint16_t port = absl::GetFlag(FLAGS_port);
  const std::string address = absl::StrCat("127.0.0.1:", port);

  grpc::reflection::InitProtoReflectionServerBuilderPlugin();

  std::signal(SIGINT, sigint_handler);
  GimliServiceImpl gimli_service;
  PublishBuildEventCallbackServiceImpl pbes_callback_service(testdata);

  grpc::ServerBuilder builder;
  builder.AddListeningPort(address, grpc::InsecureServerCredentials());
  builder.RegisterService(&gimli_service);
  builder.RegisterService(&pbes_callback_service);
  LOG(INFO) << "Server started on " << address;
  auto server = builder.BuildAndStart();
  while (!interrupted) {
    static constexpr auto kDuration = std::chrono::milliseconds(100);
    std::this_thread::sleep_for(kDuration);
  }
  server->Shutdown();
  LOG(INFO) << "Server down on " << address;

  return 0;
}

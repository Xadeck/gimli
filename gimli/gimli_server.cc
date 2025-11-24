#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/log/globals.h"
#include "absl/log/initialize.h"
#include "absl/log/log.h"
#include "absl/strings/str_cat.h"
#include "gimli/gimli.grpc.pb.h"
#include "gimli/gimli.pb.h"
#include "google/devtools/build/v1/build_events.pb.h"
#include "google/devtools/build/v1/publish_build_event.grpc.pb.h"
#include "google/devtools/build/v1/publish_build_event.pb.h"
#include "grpcpp/ext/proto_server_reflection_plugin.h"
#include "grpcpp/grpcpp.h"
#include "src/main/java/com/google/devtools/build/lib/buildeventstream/proto/build_event_stream.pb.h"
#include <csignal>
#include <cstdint>
#include <thread>

ABSL_FLAG(uint16_t, port, 8080, "The port where to listen");

namespace google::devtools::build::v1 {
namespace {

class PublishBuildEventCallbackServiceImpl final
    : public PublishBuildEvent::CallbackService {
public:
  grpc::ServerUnaryReactor *
  PublishLifecycleEvent(grpc::CallbackServerContext *context,
                        const PublishLifecycleEventRequest *request,
                        ::google::protobuf::Empty *response) final {
    LOG(INFO) << "Received LC event";
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
      Reactor() {
        LOG(INFO) << "Received BT event";
        StartRead(&request_);
      }

      void OnReadDone(bool ok) final {
        *response_.mutable_stream_id() =
            request_.ordered_build_event().stream_id();
        response_.set_sequence_number(
            request_.ordered_build_event().sequence_number());
        StartWrite(&response_);
      }

      void OnWriteDone(bool ok) final {
        const auto &event = request_.ordered_build_event().event();
        if (event.has_component_stream_finished()) {
          Finish(grpc::Status::OK);
          return;
        }
        if (event.has_bazel_event()) {
          build_event_stream::BuildEvent build_event;
          if (event.bazel_event().UnpackTo(&build_event)) {
            LOG(INFO) << "🐱 " << build_event;
          }
        }
        StartRead(&request_);
      }

      void OnDone() final {
        LOG(INFO) << "🐱done";
        delete this;
      }

    private:
      PublishBuildToolEventStreamRequest request_;
      PublishBuildToolEventStreamResponse response_;
    };
    return new Reactor();
  }
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

  const uint16_t port = absl::GetFlag(FLAGS_port);
  const std::string address = absl::StrCat("127.0.0.1:", port);

  grpc::reflection::InitProtoReflectionServerBuilderPlugin();

  std::signal(SIGINT, sigint_handler);
  GimliServiceImpl gimli_service;
  PublishBuildEventCallbackServiceImpl pbes_callback_service;

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

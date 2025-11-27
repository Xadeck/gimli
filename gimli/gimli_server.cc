#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/log/globals.h"
#include "absl/log/initialize.h"
#include "absl/log/log.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "gimli/gimli_service_impl.h"
#include "gimli/publish_build_event_callback_service_impl.h"
#include "grpcpp/ext/proto_server_reflection_plugin.h"
#include "grpcpp/grpcpp.h"
#include <csignal>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <thread>

ABSL_FLAG(uint16_t, port, 8080, "The port where to listen");
ABSL_FLAG(bool, record, false,
          "If true, even stream are recorded in testdata.");

using gimli::GimliServiceImpl;
using gimli::PublishBuildEventCallbackServiceImpl;

namespace {
volatile std::sig_atomic_t interrupted = 0;

void sigint_handler(int signal) {
  interrupted = 1;
  std::signal(signal, SIG_DFL);
}
} // namespace

int main(int argc, char **argv) {
  GOOGLE_PROTOBUF_VERIFY_VERSION;

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

  google::protobuf::ShutdownProtobufLibrary();
  return 0;
}

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/log/globals.h"
#include "absl/log/initialize.h"
#include "absl/log/log.h"
#include "absl/strings/str_cat.h"
#include "gimli/gimli.grpc.pb.h"
#include "gimli/gimli.pb.h"
#include "grpcpp/grpcpp.h"

ABSL_FLAG(int32_t, port, 8080, "The port where to listen");

namespace gimli {
namespace {

class GimliServiceImpl final : public Gimli::Service {
public:
  grpc::Status Notify(grpc::ServerContext *context,
                      const NotifyRequest *request,
                      NotifyResponse *response) final {
    LOG(INFO) << "RECEIVED " << request->input();

    response->set_output("PAPAGEI " + request->input());
    return grpc::Status::OK;
  }
};

} // namespace
} // namespace gimli

int main(int argc, char **argv) {
  absl::InitializeLog();
  absl::SetStderrThreshold(absl::LogSeverityAtLeast::kInfo);
  absl::ParseCommandLine(argc, argv);
  const int32_t port = absl::GetFlag(FLAGS_port);
  const std::string address = absl::StrCat("127.0.0.1:", port);

  gimli::GimliServiceImpl service;

  grpc::ServerBuilder builder;
  builder.AddListeningPort(address, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);
  LOG(INFO) << "Server started on " << address;
  auto server = builder.BuildAndStart();
  server->Wait();
  LOG(INFO) << "Server down on " << address;

  return 0;
}

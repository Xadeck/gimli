#include "gimli/gimli.grpc.pb.h"
#include "gimli/gimli.pb.h"
#include "grpcpp/grpcpp.h"

namespace gimli {
namespace {

class GimliServiceImpl final : public Gimli::Service {
public:
  grpc::Status Notify(grpc::ServerContext *context,
                      const NotifyRequest *request,
                      NotifyResponse *response) final {
    std::cout << "RECEIVED " << request->input();

    response->set_output("FOOBAR " + request->input());
    return grpc::Status::OK;
  }
};

} // namespace
} // namespace gimli

int main() {
  gimli::GimliServiceImpl service;

  grpc::ServerBuilder builder;
  builder.AddListeningPort("127.0.0.1:8080", grpc::InsecureServerCredentials());
  builder.RegisterService(&service);
  std::cout << "Server started\n";
  auto server = builder.BuildAndStart();
  server->Wait();

  return 0;
}

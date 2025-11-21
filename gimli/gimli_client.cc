#include "gimli/gimli.grpc.pb.h"
#include "gimli/gimli.pb.h"
#include "grpcpp/grpcpp.h"

int main() {
  auto channel =
      grpc::CreateChannel("127.0.0.1:8080", grpc::InsecureChannelCredentials());
  auto stub = gimli::Gimli::NewStub(channel);

  grpc::ClientContext context;
  gimli::NotifyRequest request;
  gimli::NotifyResponse response;

  request.set_input("hello");
  auto status = stub->Notify(&context, request, &response);
  if (!status.ok()) {
    std::cerr << status.error_message();
    return 1;
  }

  std::cout << response.output();
  return 0;
}

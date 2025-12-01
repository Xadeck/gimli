#include <cstdint>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "gimli/gimli.grpc.pb.h"
#include "gimli/gimli.pb.h"
#include "grpcpp/grpcpp.h"

ABSL_FLAG(uint16_t, port, 8080, "The port where to listen");

int main(int argc, char** argv) {
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  absl::ParseCommandLine(argc, argv);
  const uint16_t port = absl::GetFlag(FLAGS_port);
  const std::string address = absl::StrCat("127.0.0.1:", port);

  auto channel =
    grpc::CreateChannel(address, grpc::InsecureChannelCredentials());
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

  google::protobuf::ShutdownProtobufLibrary();
  return 0;
}

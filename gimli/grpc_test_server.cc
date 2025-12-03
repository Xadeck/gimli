#include "gimli/grpc_test_server.h"

#include "absl/strings/str_cat.h"
#include "grpcpp/security/server_credentials.h"

namespace gimli {

TestServer::Builder::Builder() {
  server_builder_.AddListeningPort(
    "localhost:0", grpc::InsecureServerCredentials(), &port_selected_);
}

TestServer::Builder&& TestServer::Builder::RegisterService(
  grpc::Service* service) && {
  server_builder_.RegisterService(service);
  return std::move(*this);
}

TestServer TestServer::Builder::BuildAndStart() && {
  auto server = server_builder_.BuildAndStart();
  auto channel = grpc::CreateChannel(absl::StrCat("localhost:", port_selected_),
                                     grpc::InsecureChannelCredentials());
  return TestServer(std::move(server), std::move(channel));
};

void TestServer::Shutdown() && {
  server_->Shutdown();
  server_->Wait();
}

}  // namespace gimli

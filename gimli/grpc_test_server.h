#ifndef _GIMLI_GRPC_TEST_SERVER_H_
#define _GIMLI_GRPC_TEST_SERVER_H_

#include <memory>

#include "grpcpp/grpcpp.h"

namespace gimli {

class TestServer {
 public:
  class Builder {
   public:
    Builder();
    Builder&& RegisterService(grpc::Service* service) &&;
    TestServer BuildAndStart() &&;
    ;

   private:
    grpc::ServerBuilder server_builder_;
    int port_selected_ = 0;
  };

  template <typename T>
  std::unique_ptr<typename T::Stub> NewStub() {
    return T::NewStub(channel_);
  }

  void Shutdown() &&;

 private:
  friend class Builder;
  TestServer(std::unique_ptr<grpc::Server> server,
             std::shared_ptr<grpc::Channel> channel)
    : server_(std::move(server)), channel_(std::move(channel)) {}
  std::unique_ptr<grpc::Server> server_;
  std::shared_ptr<grpc::Channel> channel_;
};

}  // namespace gimli

#endif  // _GIMLI_GRPC_TEST_SERVER_H_

#include "gimli/gimli_service_impl.h"

#include "gimli/gimli.pb.h"
#include "grpcpp/grpcpp.h"

namespace gimli {

grpc::Status GimliServiceImpl::Notify(grpc::ServerContext* context,
                                      const NotifyRequest* request,
                                      NotifyResponse* response) {
  response->set_output("PAPAGEI " + request->input());
  return grpc::Status::OK;
}

}  // namespace gimli

#ifndef _GIMLI_SERVICE_IMPL_H_
#define _GIMLI_SERVICE_IMPL_H_

#include "gimli/gimli.grpc.pb.h"
#include "gimli/gimli.pb.h"

namespace gimli {

class GimliServiceImpl final : public Gimli::Service {
public:
  grpc::Status Notify(grpc::ServerContext *context,
                      const NotifyRequest *request,
                      NotifyResponse *response) final;
};

} // namespace gimli

#endif // _GIMLI_SERVICE_IMPL_H_

#ifndef _GIMLI_SERVICE_IMPL_H_
#define _GIMLI_SERVICE_IMPL_H_

#include "absl/base/nullability.h"
#include "gimli/gimli.grpc.pb.h"
#include "gimli/gimli.pb.h"
#include "gimli/reporter.h"

namespace gimli {

class GimliServiceImpl final : public proto::Gimli::Service {
 public:
  GimliServiceImpl(const Reporter* absl_nonnull reporter);

  grpc::Status GetReport(grpc::ServerContext* absl_nonnull context,
                         const proto::GetReportRequest* absl_nonnull request,
                         proto::GetReportResponse* absl_nonnull response) final;

 private:
  const Reporter* absl_nonnull reporter_;
};

}  // namespace gimli

#endif  // _GIMLI_SERVICE_IMPL_H_

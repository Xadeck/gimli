#include "gimli/gimli_service_impl.h"

#include "absl/base/nullability.h"
#include "absl/strings/substitute.h"
#include "gimli/gimli.pb.h"
#include "google/protobuf/util/time_util.h"
#include "grpcpp/grpcpp.h"

namespace gimli {
using ::google::protobuf::util::TimeUtil;

GimliServiceImpl::GimliServiceImpl(const Reporter* absl_nonnull reporter)
  : reporter_(reporter) {}

grpc::Status GimliServiceImpl::GetReport(
  grpc::ServerContext* absl_nonnull context,
  const proto::GetReportRequest* absl_nonnull request,
  proto::GetReportResponse* absl_nonnull response) {
  if (!request->has_workspace_path()) {
    return {grpc::StatusCode::INVALID_ARGUMENT,
            "missing `workspace_path` in request"};
  }

  const auto report = reporter_->GetReportFor(request->workspace_path());
  if (!report.has_value()) {
    return {grpc::StatusCode::NOT_FOUND,
            absl::Substitute("No report for workspace `$0`",
                             request->workspace_path())};
  }
  auto& report_proto = *response->mutable_report();
  *report_proto.mutable_time() =
    TimeUtil::NanosecondsToTimestamp(absl::ToUnixNanos(report->time));

  for (const auto& error : report->errors) {
    auto& error_proto = *report_proto.add_errors();
    error_proto.set_path_in_workspace(error.path_in_workspace);
    error_proto.set_line(error.line);
    if (error.column != -1) error_proto.set_column(error.column);
    error_proto.set_message(error.message);
    for (const auto& context : error.context) {
      error_proto.add_context(context);
    }
  }

  return grpc::Status::OK;
}

}  // namespace gimli

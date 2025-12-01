#ifndef _GIMLI_PUBLISH_BUILD_EVENT_CALLBACK_SERVICE_IMPL_H_
#define _GIMLI_PUBLISH_BUILD_EVENT_CALLBACK_SERVICE_IMPL_H_

#include <optional>

#include "absl/base/nullability.h"
#include "gimli/reporter.h"
#include "gimli/stderr_processor.h"
#include "google/devtools/build/v1/publish_build_event.grpc.pb.h"
#include "google/devtools/build/v1/publish_build_event.pb.h"

namespace gimli {

class PublishBuildEventCallbackServiceImpl final
  : public google::devtools::build::v1::PublishBuildEvent::CallbackService {
 public:
  // Reporter's scope must encompass the scope of this object.
  PublishBuildEventCallbackServiceImpl(
    Reporter& reporter, std::optional<std::filesystem::path> testdata);

  grpc::ServerUnaryReactor* PublishLifecycleEvent(
    grpc::CallbackServerContext* context,
    const google::devtools::build::v1::PublishLifecycleEventRequest* request,
    google::protobuf::Empty* response) final;

  grpc::ServerBidiReactor<
    google::devtools::build::v1::PublishBuildToolEventStreamRequest,
    google::devtools::build::v1::PublishBuildToolEventStreamResponse>*
  PublishBuildToolEventStream(grpc::CallbackServerContext* context) final;

 private:
  Reporter* absl_nonnull reporter_;
  StderrProcessor stderr_processor_;
  std::optional<std::filesystem::path> testdata_;
};

}  // namespace gimli

#endif  // _GIMLI_PUBLISH_BUILD_EVENT_CALLBACK_SERVICE_IMPL_H_

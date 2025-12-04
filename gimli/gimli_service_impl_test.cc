#include "gimli/gimli_service_impl.h"

#include "gimli/gimli.grpc.pb.h"
#include "gimli/gimli.pb.h"
#include "gimli/grpc_test_server.h"
#include "gimli/reporter.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "protobuf-matchers/protocol-buffer-matchers.h"

namespace gimli {
namespace {
using ::protobuf_matchers::EqualsProto;

class GimliServiceImplTest : public testing::Test {
 protected:
  ~GimliServiceImplTest() { std::move(test_server_).Shutdown(); }

  Reporter reporter_;
  GimliServiceImpl under_test_{&reporter_};

  TestServer test_server_ =
    TestServer::Builder().RegisterService(&under_test_).BuildAndStart();
  std::unique_ptr<proto::Gimli::Stub> stub_ =
    test_server_.NewStub<proto::Gimli>();
};

TEST_F(GimliServiceImplTest, ReturnsErrorForInvalidRequest) {
  grpc::ClientContext context;
  proto::GetReportRequest request;
  proto::GetReportResponse response;

  auto status = stub_->GetReport(&context, request, &response);
  ASSERT_EQ(status.error_code(), grpc::StatusCode::INVALID_ARGUMENT);
  ASSERT_EQ(status.error_message(), R"(missing `workspace_path` in request)");
}

TEST_F(GimliServiceImplTest, ReturnsErrorForRelativeWorkspace) {
  grpc::ClientContext context;
  proto::GetReportRequest request;
  proto::GetReportResponse response;

  request.set_workspace_path("some/project");
  auto status = stub_->GetReport(&context, request, &response);
  ASSERT_EQ(status.error_code(), grpc::StatusCode::INVALID_ARGUMENT);
  ASSERT_EQ(status.error_message(), R"(`workspace_path` must be absolute)");
}

TEST_F(GimliServiceImplTest, ReturnsErrorForNotFoundWorkspace) {
  grpc::ClientContext context;
  proto::GetReportRequest request;
  proto::GetReportResponse response;

  request.set_workspace_path("/not/existing");
  auto status = stub_->GetReport(&context, request, &response);
  ASSERT_EQ(status.error_code(), grpc::StatusCode::NOT_FOUND);
}

TEST_F(GimliServiceImplTest, ReturnsReportWhenExisting) {
  constexpr int64_t kKnownTime = 1764863274;
  reporter_.AddReport({
    .workspace_path = "/some/project",
    .time = absl::FromUnixMicros(kKnownTime),
    .errors =
      {
        {
          .path_in_workspace = "main.cc",
          .line = 5,
          .message = "Problem",
          .context =
            {
              "Here...",
              "...or there",
            },
        },
      },
  });

  grpc::ClientContext context;
  proto::GetReportRequest request;
  proto::GetReportResponse response;

  request.set_workspace_path("/some/project");
  const auto status = stub_->GetReport(&context, request, &response);
  ASSERT_TRUE(status.ok()) << status.error_message();
  EXPECT_THAT(response,
              EqualsProto(R"pb(report {
                                 time { seconds: 1764 nanos: 863274000 }
                                 errors {
                                   path_in_workspace: "main.cc"
                                   line: 5
                                   message: "Problem"
                                   context: "Here..."
                                   context: "...or there"
                                 }
                               })pb"));
}

}  // namespace
}  // namespace gimli

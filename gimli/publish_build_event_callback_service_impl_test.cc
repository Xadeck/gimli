#include "gimli/publish_build_event_callback_service_impl.h"

#include <cstddef>

#include "absl/status/status_matchers.h"
#include "absl/time/time.h"
#include "gimli/grpc_test_server.h"
#include "gimli/gtest_runfiles.h"
#include "gimli/recording.pb.h"
#include "gmock/gmock.h"
#include "google/devtools/build/v1/publish_build_event.pb.h"
#include "google/protobuf/text_format.h"
#include "grpcpp/grpcpp.h"
#include "gtest/gtest.h"

namespace gimli {
namespace {
using ::absl_testing::IsOk;
using ::google::devtools::build::v1::PublishBuildEvent;
using ::google::devtools::build::v1::PublishBuildToolEventStreamResponse;
using ::testing::ElementsAreArray;
using ::testing::SizeIs;

TEST(PublishBuildEventCallbackServiceImplTest, Works) {
  // Read the recording from testdata.
  auto data = Runfiles::ContentsOf("gimli/testdata/non_fatal_error.textproto");
  ASSERT_THAT(data, IsOk());
  gimli::Recording recording;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(*data, &recording));

  // Create a server with the service to be tested. This selects a random port
  // automatically, which we save for connecting to it later.
  Reporter reporter;
  PublishBuildEventCallbackServiceImpl under_test(reporter, std::nullopt);

  auto test_server =
    TestServer::Builder().RegisterService(&under_test).BuildAndStart();
  auto stub = test_server.NewStub<PublishBuildEvent>();

  // Write all recorded requests.
  grpc::ClientContext context;
  auto stream = stub->PublishBuildToolEventStream(&context);
  for (const auto& request : recording.requests()) {
    EXPECT_TRUE(stream->Write(request));
  }
  stream->WritesDone();

  // Read all recorded responses, otherwise the stream can't be closed (the
  // server is still waiting for what it returned to be consummed).
  //
  // Technically, we should check that writes and leaves are interleaved. But
  // it is simpler to do all writes, then all reads and check there are the
  // same numbers. It's a good enough approximation for the purpose of
  // exercising the server in a test.
  size_t responses_count = 0;
  PublishBuildToolEventStreamResponse response;
  while (stream->Read(&response)) {
    ++responses_count;
  }
  EXPECT_EQ(responses_count, recording.requests_size());
  // Close the stream.
  stream->Finish();

  std::move(test_server).Shutdown();

  // Check that the reporter has a report
  auto report = reporter.GetReportFor("/Users/xdecoret/gimli");
  ASSERT_TRUE(report.has_value());
  ASSERT_EQ(report->workspace_path, "/Users/xdecoret/gimli");

  // The report time is known from the testdata: grep for `start_time`,
  // ignore the deprecated `start_time_millis`.
  ASSERT_EQ(report->time,
            absl::FromUnixSeconds(1764368148) + absl::Nanoseconds(324000000));
  ASSERT_THAT(report->errors, SizeIs(1));
  EXPECT_EQ(report->errors[0].path_in_workspace,
            "gimli/testdata/non_fatal_error.cc");
  EXPECT_EQ(report->errors[0].line, 6);
  EXPECT_EQ(report->errors[0].column, 16);
  EXPECT_EQ(report->errors[0].message,
            "error: use of undeclared identifier 'y'");
  EXPECT_THAT(report->errors[0].context,
              ElementsAreArray({
                R"(    6 |   std::cout << y << std::endl;)",
                R"(      |                ^)",
              }));
}

}  // namespace
}  // namespace gimli

#include "absl/log/globals.h"
#include "absl/log/initialize.h"
#include "absl/strings/str_cat.h"
#include "gimli/publish_build_event_callback_service_impl.h"
#include "gimli/recording.pb.h"
#include "google/devtools/build/v1/publish_build_event.pb.h"
#include "google/protobuf/text_format.h"
#include "grpcpp/grpcpp.h"
#include "gtest/gtest.h"
#include <fstream>
#include <gmock/gmock.h>

namespace gimli {
namespace {

using ::google::devtools::build::v1::PublishBuildEvent;
using ::google::devtools::build::v1::PublishBuildToolEventStreamResponse;

class PublishBuildEventCallbackServiceImplTest : public testing::Test {
protected:
  static void SetUpTestCase() {
    absl::InitializeLog();
    absl::SetStderrThreshold(absl::LogSeverityAtLeast::kInfo);
  }
};

std::optional<std::string> GetData(std::filesystem::path workspace_path) {
  std::ifstream stream(workspace_path);
  if (!stream.is_open()) {
    return std::nullopt;
  }

  return std::string(std::istreambuf_iterator<char>(stream),
                     std::istreambuf_iterator<char>());
}

TEST_F(PublishBuildEventCallbackServiceImplTest, Works) {
  // Read the recording from testdata.
  auto data = GetData("gimli/testdata/non_fatal_error.textproto");
  ASSERT_TRUE(data.has_value());
  gimli::Recording recording;
  ASSERT_TRUE(google::protobuf::TextFormat::ParseFromString(*data, &recording));

  // Create a server with the service to be tested. This selects a random port
  // automatically, which we save for connecting to it later.
  PublishBuildEventCallbackServiceImpl under_test(std::nullopt);
  int port_selected = -1;
  grpc::ServerBuilder builder;
  builder.AddListeningPort("localhost:0", grpc::InsecureServerCredentials(),
                           &port_selected);
  builder.RegisterService(&under_test);
  auto server = builder.BuildAndStart();

  // Creates a channel to the server, and a stub to the service.
  auto channel = grpc::CreateChannel(absl::StrCat("localhost:", port_selected),
                                     grpc::InsecureChannelCredentials());
  auto stub = PublishBuildEvent::NewStub(channel);

  // Write all recorded requests.
  grpc::ClientContext context;
  auto stream = stub->PublishBuildToolEventStream(&context);
  for (const auto &request : recording.requests()) {
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

  // Shutdown the server and wait for termination.
  server->Shutdown();
  server->Wait();
}

} // namespace
} // namespace gimli

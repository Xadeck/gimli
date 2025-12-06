#include <cstdint>
#include <filesystem>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "gimli/gimli.grpc.pb.h"
#include "gimli/gimli.pb.h"
#include "grpcpp/grpcpp.h"

ABSL_FLAG(uint16_t, port, 9090, "The port on which ");
ABSL_FLAG(
  std::optional<std::string>, path, std::nullopt,
  R"(If set, retrieves the report for the workspace containing this file.)"
  R"(If not set, retreives the report for the current working directory.)");

int main(int argc, char** argv) {
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  absl::ParseCommandLine(argc, argv);
  const uint16_t port = absl::GetFlag(FLAGS_port);
  const std::string address = absl::StrCat("127.0.0.1:", port);

  auto channel =
    grpc::CreateChannel(address, grpc::InsecureChannelCredentials());
  auto stub = gimli::proto::Gimli::NewStub(channel);

  grpc::ClientContext context;
  gimli::proto::GetReportRequest request;
  gimli::proto::GetReportResponse response;

  request.set_path(
    absl::GetFlag(FLAGS_path).value_or(std::filesystem::current_path()));

  auto status = stub->GetReport(&context, request, &response);
  if (!status.ok()) {
    std::cerr << status.error_message();
    return 1;
  }

  std::cout << response.DebugString();

  google::protobuf::ShutdownProtobufLibrary();
  return 0;
}

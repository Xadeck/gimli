#include "gimli/gtest_runfiles.h"

#include <fstream>
#include <string>

#include "absl/log/log.h"
#include "absl/status/status.h"
#include "absl/strings/str_cat.h"
#include "gtest/gtest.h"
#include "rules_cc/cc/runfiles/runfiles.h"

namespace gimli {
namespace {

class RunfilesEnvironment final : public ::testing::Environment {
 public:
  void SetUp() final {
    std::string error;
    runfiles = ::rules_cc::cc::runfiles::Runfiles::CreateForTest(&error);
    ASSERT_NE(runfiles, nullptr) << error;
  }

  void TearDown() final { delete runfiles; }

  ::rules_cc::cc::runfiles::Runfiles* runfiles = nullptr;
};

auto* const env = dynamic_cast<RunfilesEnvironment*>(
  testing::AddGlobalTestEnvironment(new RunfilesEnvironment));

}  // namespace

absl::StatusOr<std::string> Runfiles::ContentsOf(
  std::filesystem::path workspace_path) {
  // This matches the name in MODULE.bazel
  static const std::filesystem::path kProjectPath = "gimli";
  // https://fuchsia.googlesource.com/fuchsia/+/HEAD/build/bazel/BAZEL_RUNFILES.md
  auto rlocation = env->runfiles->Rlocation(kProjectPath / workspace_path);
  std::ifstream stream(rlocation);
  if (!stream.is_open()) {
    LOG(ERROR) << "File not found " << rlocation;
    return absl::NotFoundError(absl::StrCat(
      std::string(workspace_path),
      " not found, did your forget to add it to the `data` of the rule?"));
  }

  return std::string(std::istreambuf_iterator<char>(stream),
                     std::istreambuf_iterator<char>());
}

}  // namespace gimli

#include "absl/base/log_severity.h"
#include "absl/log/globals.h"
#include "absl/log/initialize.h"
#include "gtest/gtest.h"

namespace {

class LogEnvironment final : public ::testing::Environment {
 public:
  void SetUp() final {
    absl::InitializeLog();
    absl::SetStderrThreshold(absl::LogSeverityAtLeast::kInfo);
  }

  void TearDown() final {}
};

auto* const env = testing::AddGlobalTestEnvironment(new LogEnvironment);

}  // namespace

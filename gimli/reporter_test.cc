#include "gimli/reporter.h"

#include "absl/time/clock.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace gimli {
namespace {
using ::testing::Eq;
using ::testing::IsEmpty;
using ::testing::SizeIs;

TEST(ReporterTest, Works) {
  Reporter under_test;

  // Initially we have no report.
  ASSERT_THAT(under_test.GetReportFor("/some/project"), Eq(std::nullopt));

  // Adding a report will make it available.
  under_test.AddReport({
    .workspace_path = "/some/project",
    .time = absl::Now(),
    .errors =
      {
        {
          .path_in_workspace = "main.cc",
          .line = 5,
          .contents =
            {
              "This is an error",
              "on two lines",
            },
        },
      },
  });

  // Check that it's indeed available. Check only a few fields.
  auto report = under_test.GetReportFor("/some/project");
  ASSERT_TRUE(report.has_value());
  ASSERT_THAT(report->errors, SizeIs(1));
  auto error = report->errors.front();
  ASSERT_THAT(error.path_in_workspace, Eq("main.cc"));

  // Adding a new report overrides the previous one.
  under_test.AddReport({
    .workspace_path = "/some/project",
    .time = absl::Now(),
  });
  report = under_test.GetReportFor("/some/project");
  ASSERT_TRUE(report.has_value());
  ASSERT_THAT(report->errors, IsEmpty());
}

}  // namespace
}  // namespace gimli

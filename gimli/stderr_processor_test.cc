#include "gimli/stderr_processor.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace gimli {
namespace {
using ::testing::ElementsAre;
using ::testing::ElementsAreArray;
using ::testing::SizeIs;
using ::testing::StrEq;

TEST(StderrProcessorTest, Works) {
  const std::string_view stderr =
    "\r\033[1A\033[K\033[31m\033[1mERROR: "
    "\033[0m/Users/xdecoret/gimli/gimli/testdata/BUILD:5:10: Compiling "
    "gimli/testdata/non_fatal_error.cc failed: (Exit 1): cc_wrapper.sh failed: "
    "error executing CppCompile command (from target "
    "//gimli/testdata:non_fatal_error) "
    "external/rules_cc++cc_configure_extension+local_config_cc/cc_wrapper.sh "
    "-U_FORTIFY_SOURCE -fstack-protector -Wall -Wthread-safety -Wself-assign "
    "-Wunused-but-set-parameter -Wno-free-nonheap-object ... (remaining 29 "
    "arguments skipped)\n\nUse --sandbox_debug to see verbose messages from "
    "the sandbox and retain the sandbox build root for "
    "debugging\n\033[1mgimli/testdata/non_fatal_error.cc:6:16: "
    "\033[0m\033[0;1;31merror: \033[0m\033[1muse of undeclared identifier "
    "\'y\'\033[0m\n    6 |   std::cout << y << std::endl;\033[0m\n      | "
    "\033[0;1;32m               ^\n\033[0m1 error generated.\n\033[32m[2 / "
    "4]\033[0m Compiling gimli/testdata/non_fatal_error.cc; 0s "
    "darwin-sandbox\n";

  StderrProcessor under_test;
  ASSERT_THAT(
    under_test.ToContents(stderr),
    ElementsAreArray({
      R"(ERROR: /Users/xdecoret/gimli/gimli/testdata/BUILD:5:10: Compiling gimli/testdata/non_fatal_error.cc failed: (Exit 1): cc_wrapper.sh failed: error executing CppCompile command (from target //gimli/testdata:non_fatal_error) external/rules_cc++cc_configure_extension+local_config_cc/cc_wrapper.sh -U_FORTIFY_SOURCE -fstack-protector -Wall -Wthread-safety -Wself-assign -Wunused-but-set-parameter -Wno-free-nonheap-object ... (remaining 29 arguments skipped))",
      R"(Use --sandbox_debug to see verbose messages from the sandbox and retain )"
      R"(the sandbox build root for debugging)",
      R"(gimli/testdata/non_fatal_error.cc:6:16: error: use of undeclared identifier 'y')",
      R"(    6 |   std::cout << y << std::endl;)",
      R"(      |                ^)",
      R"(1 error generated.)",
      R"([2 / 4] Compiling gimli/testdata/non_fatal_error.cc; 0s darwin-sandbox)",
    }));

  auto errors = under_test.ToErrors(stderr);
  ASSERT_THAT(errors, SizeIs(1));
  EXPECT_EQ(errors[0].path_in_workspace, "gimli/testdata/non_fatal_error.cc");
  EXPECT_EQ(errors[0].line, 6);
  EXPECT_EQ(errors[0].column, 16);
  EXPECT_EQ(errors[0].message, "error: use of undeclared identifier 'y'");
  EXPECT_THAT(errors[0].context, ElementsAreArray({
                                   R"(    6 |   std::cout << y << std::endl;)",
                                   R"(      |                ^)",
                                 }));
}

}  // namespace
}  // namespace gimli

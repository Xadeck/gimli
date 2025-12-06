#ifndef _GIMLI_GTEST_RUNFILES_H_
#define _GIMLI_GTEST_RUNFILES_H_

#include <filesystem>
#include <string>

#include "absl/status/statusor.h"

namespace gimli {

struct Runfiles {
  // Returns the contents of a resource, which must be in the `data` argument
  // of the build target, or an error.
  static absl::StatusOr<std::string> ContentsOf(
    std::filesystem::path workspace_path);
};

}  // namespace gimli

#endif  // _GIMLI_GTEST_RUNFILES_H_

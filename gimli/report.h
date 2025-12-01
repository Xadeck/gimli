#ifndef _GIMLI_REPORT_H_
#define _GIMLI_REPORT_H_

#include <filesystem>
#include <string>
#include <vector>

#include "absl/time/time.h"

namespace gimli {

// List of errors (if any) for a Bazel build in a given workspace.
struct Report {
  // Represent a compilation error detected
  struct Error {
    // Path of the file where error occured, inside the workspace for a report.
    std::filesystem::path path_in_workspace;
    // Line where error occured.
    int line;
    // Lines of the error message.
    std::vector<std::string> contents;
  };

  std::filesystem::path workspace_path;
  absl::Time time;
  std::vector<Error> errors;
};

}  // namespace gimli

#endif  // _GIMLI_REPORT_H_

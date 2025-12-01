#include "gimli/stderr_processor.h"

#include <regex>
#include <string>
#include <string_view>

#include "absl/strings/str_split.h"

namespace gimli {

std::vector<std::string> StderrProcessor::ToContents(
  std::string_view stderr) const {
  return absl::StrSplit(
    std::regex_replace(std::string(stderr), ansi_codes_, ""),
    absl::ByAnyChar("\r\n"), absl::SkipEmpty());
}

std::vector<Report::Error> StderrProcessor::ToErrors(
  std::string_view stderr) const {
  std::vector<Report::Error> errors;
  Report::Error* ongoing_error = nullptr;

  auto contents = ToContents(stderr);
  for (const std::string& line : contents) {
    std::smatch match;
    if (std::regex_match(line, match, error_begin_pattern_)) {
      errors.push_back({
        .path_in_workspace = match[1].str(),
        .line = std::stoi(match[2].str()),
        .column = std::stoi(match[3].str()),
        .message = match[4].str(),
      });
      ongoing_error = &errors.back();
      continue;
    }
    if (std::regex_match(line, error_end_pattern_)) {
      ongoing_error = nullptr;
      continue;
    }
    if (ongoing_error != nullptr) {
      ongoing_error->context.push_back(line);
      continue;
    }
  }
  return errors;
}

}  // namespace gimli

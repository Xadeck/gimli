#ifndef _USERS_XDECORET_GIMLI_GIMLI_STDERR_PROCESSOR_H_
#define _USERS_XDECORET_GIMLI_GIMLI_STDERR_PROCESSOR_H_

#include <regex>
#include <string>
#include <string_view>
#include <vector>

#include "gimli/report.h"

namespace gimli {

class StderrProcessor {
 public:
  std::vector<std::string> ToContents(std::string_view stderr) const;
  std::vector<Report::Error> ToErrors(std::string_view stderr) const;

 private:
  // Regex explanation:
  // \x1b     -> Matches the ESC character (ASCII 27)
  // \[       -> Matches the literal '[' that follows ESC in CSI sequences
  // [0-9;?]* -> Matches zero or more parameter bytes (digits, semicolons,
  // question marks) [ -/]* -> Matches zero or more intermediate bytes
  // [@-~]    -> Matches the final byte (usually a letter like 'm', 'K', etc.)
  std::regex ansi_codes_{R"(\x1b\[[0-9;?]*[ -/]*[@-~])"};
  std::regex error_begin_pattern_{R"(^([^:]+?):(\d+):(\d+): (.+)$)"};
  std::regex error_end_pattern_{R"(1 error generated.)"};
};

}  // namespace gimli

#endif  // _USERS_XDECORET_GIMLI_GIMLI_STDERR_PROCESSOR_H_

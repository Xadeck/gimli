#ifndef _USERS_XDECORET_GIMLI_GIMLI_REPORTER_H_
#define _USERS_XDECORET_GIMLI_GIMLI_REPORTER_H_

#include <filesystem>
#include <mutex>
#include <optional>
#include <unordered_map>

#include "gimli/report.h"

namespace gimli {

class Reporter {
 public:
  // Add a report. Any report for the same workspace will be replaced.
  void AddReport(Report report);

  std::optional<Report> GetReportFor(
    std::filesystem::path workspace_path) const;

 private:
  mutable std::mutex mutex_;
  std::unordered_map<std::filesystem::path, Report> per_workspace_path_reports_;
};

}  // namespace gimli

#endif  // _USERS_XDECORET_GIMLI_GIMLI_REPORTER_H_

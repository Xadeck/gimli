#include "gimli/reporter.h"

#include <mutex>
#include <utility>

namespace gimli {

void Reporter::AddReport(Report report) {
  std::scoped_lock lock(mutex_);
  per_workspace_path_reports_[report.workspace_path] = std::move(report);
}

std::optional<Report> Reporter::GetReportFor(
  std::filesystem::path workspace_path) const {
  std::scoped_lock lock(mutex_);
  for (const auto& [key, report] : per_workspace_path_reports_) {
    if (key == workspace_path) return report;
  }
  return std::nullopt;
}

}  // namespace gimli

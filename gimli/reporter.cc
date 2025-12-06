#include "gimli/reporter.h"

#include <algorithm>
#include <filesystem>
#include <mutex>
#include <utility>

namespace gimli {
namespace {

bool is_subpath(const std::filesystem::path& path,
                const std::filesystem::path& base) {
  const auto pair =
    std::mismatch(path.begin(), path.end(), base.begin(), base.end());
  return (pair.second == base.end());
}

}  // namespace

void Reporter::AddReport(Report report) {
  std::scoped_lock lock(mutex_);
  per_workspace_path_reports_[(report.workspace_path).lexically_normal()] =
    std::move(report);
}

std::optional<Report> Reporter::GetReportFor(
  std::filesystem::path workspace_path) const {
  std::scoped_lock lock(mutex_);
  const auto search_key = (workspace_path).lexically_normal();
  for (const auto& [key, report] : per_workspace_path_reports_) {
    if (is_subpath(search_key, key)) return report;
  }
  return std::nullopt;
}

}  // namespace gimli

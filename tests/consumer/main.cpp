#include <gtest-criteria/criteria_policy.hpp>
#include <gtest-criteria/sqlite_backend.hpp>

#include <chrono>
#include <filesystem>

int main() {
    gtest_criteria::CriteriaOptions options;
    if (gtest_criteria::IsPerformanceDeviation(
            100.0, 100.0, 1.0, options.minimum_history_count, options)) {
        return 1;
    }

    const auto database_path =
        std::filesystem::temp_directory_path() /
        ("gtest-criteria-consumer-" +
         std::to_string(
             std::chrono::steady_clock::now().time_since_epoch().count()) +
         ".db");
    {
        testing::SqliteBackend database(database_path);
    }
    std::error_code error;
    std::filesystem::remove(database_path, error);
    return error ? 1 : 0;
}

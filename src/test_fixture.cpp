

#include <cmath>
#include <filesystem>
#include "sqlite3.h"
#include "test_fixture.hpp"

sqlite3* GTestCriteriaFixture::m_db = nullptr;

void GTestCriteriaFixture::ConfigureSqlDb(std::optional<SqlOutput> arg) {
    if (arg->type == SqlOutputType::SQLite) {
        const std::filesystem::path db_path = arg->connection;
        if (!std::filesystem::exists(db_path)) {
            throw std::runtime_error("Database path does not exist: " + db_path.string());
        }

        const int result = sqlite3_open_v2(db_path.string().c_str(), &m_db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr);
        if (result != SQLITE_OK) {
            const std::string error = m_db != nullptr ? sqlite3_errmsg(m_db) : "unable to allocate SQLite handle";
            if (m_db != nullptr) {
                sqlite3_close(m_db);
                m_db = nullptr;
            }
            throw std::runtime_error("Unable to open SQLite database '" + db_path.string() + "': " + error);
        }
    }
}

GTestCriteriaFixture::DurationStats GTestCriteriaFixture::ComputeDurationStats(
    const std::string& suite_name, const std::string& test_name) {
    DurationStats stats;
    if (m_db == nullptr) {
        return stats;
    }

    // AVG(diff) and AVG(diff * diff) let us derive the population standard
    // deviation (variance = E[x^2] - E[x]^2) with a single aggregate query.
    // Join to "suite" so the caller can select a specific suite/test pair
    // rather than aggregating across every suite that happens to share a
    // test name.
    const char* sql =
        "SELECT AVG(test.end_timestamp - test.start_timestamp), "
        "AVG((test.end_timestamp - test.start_timestamp) * "
        "(test.end_timestamp - test.start_timestamp)), COUNT(*) "
        "FROM test JOIN suite ON test.suite_id = suite.id "
        "WHERE suite.name = ? AND test.name = ? "
        "AND test.start_timestamp IS NOT NULL AND test.end_timestamp IS NOT NULL;";

    sqlite3_stmt* statement = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &statement, nullptr) != SQLITE_OK) {
        return stats;
    }

    sqlite3_bind_text(statement, 1, suite_name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(statement, 2, test_name.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(statement) == SQLITE_ROW) {
        stats.count = sqlite3_column_int64(statement, 2);
        if (stats.count > 0) {
            stats.mean = sqlite3_column_double(statement, 0);
            const double mean_of_squares = sqlite3_column_double(statement, 1);
            const double variance = mean_of_squares - stats.mean * stats.mean;
            // Clamp tiny negative values caused by floating-point rounding.
            stats.stddev = variance > 0.0 ? std::sqrt(variance) : 0.0;
        }
    }
    sqlite3_finalize(statement);
    return stats;
}



void GTestCriteriaFixture::SetUp()
{
    test_timer = std::chrono::high_resolution_clock::now();
}

void GTestCriteriaFixture::EXPECT_PERFORMANCE(DurationStats stats, std::chrono::high_resolution_clock::duration test_duration)
{
    if (stats.count == 0) {
        return;
    }

    // Example performance expectation: test duration should not exceed mean + 2 * stddev
    const double threshold = stats.mean + 2 * stats.stddev;
    EXPECT_LE(std::chrono::duration_cast<std::chrono::milliseconds>(test_duration).count(), threshold) << "Test duration exceeded performance threshold.";
}

void GTestCriteriaFixture::TearDown()
{
    auto test_duration = std::chrono::high_resolution_clock::now() - test_timer;
    const auto test_duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(test_duration).count();

    const testing::TestInfo* test_info = testing::UnitTest::GetInstance()->current_test_info();
    const DurationStats test_stats = ComputeDurationStats(test_info->test_suite_name(), test_info->name());

    EXPECT_PERFORMANCE(test_stats, test_duration);
}

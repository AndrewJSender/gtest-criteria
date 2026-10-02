// Copyright (c) 2026 Andrew J. Sender.
// All rights reserved.

#include <cmath>
#include <stdexcept>
#include <filesystem>

#include "test_fixture.hpp"
#include "postgresql_backend.hpp"
#include "sqlite_backend.hpp"

#ifndef GTEST_CRITERIA_SQL_DIR
#define GTEST_CRITERIA_SQL_DIR "sql/sqlite"
#endif

namespace {

const std::string kCriteriaSqlFile = std::filesystem::absolute(GTEST_CRITERIA_SQL_DIR "/criteria.sql").string();
const std::string kDisableExpectPerformantArgName = "--criteria_disable_expect_performant";
const std::string kDisableExpectIntegrityArgName = "--criteria_disable_expect_integrity";

}  // namespace

std::unique_ptr<testing::SqlBackend> GTestCriteriaFixture::m_db;
bool GTestCriteriaFixture::m_expect_performant_disabled = false;
bool GTestCriteriaFixture::m_expect_integrity_disabled = false;

void GTestCriteriaFixture::ConfigureSqlDb(std::optional<SqlOutput> arg) {
    m_db.reset();
    if (!arg.has_value()) {
        return;
    }

    if (arg->type == SqlOutputType::SQLite) {
        const std::filesystem::path db_path = arg->connection;
        if (!std::filesystem::exists(db_path)) {
            throw std::runtime_error("Database path does not exist: " + db_path.string());
        }
        m_db = std::make_unique<testing::SqliteBackend>(db_path);
    } else {
        m_db =
            std::make_unique<testing::PostgreSqlBackend>(arg->connection);
    }
}

GTestCriteriaFixture::DurationStats GTestCriteriaFixture::ComputeDurationStats(
    const std::string& suite_name, const std::string& test_name) {
    DurationStats stats;
    if (m_db == nullptr) {
        return stats;
    }

    const auto rows = m_db->Query(
        kCriteriaSqlFile.c_str(), "select_test_durations",
        {suite_name, test_name}, {});
    if (rows.size() != 1 || rows.front().size() != 3) {
        throw std::runtime_error("Invalid result from test-duration query");
    }

    stats.count = std::stoll(rows.front()[2]);
    if (stats.count > 0) {
        stats.mean = std::stod(rows.front()[0]);
        const double mean_of_squares = std::stod(rows.front()[1]);
        const double variance = mean_of_squares - stats.mean * stats.mean;
        // Clamp tiny negative values caused by floating-point rounding.
        stats.stddev = variance > 0.0 ? std::sqrt(variance) : 0.0;
    }
    return stats;
}

std::optional<std::vector<std::string>> GTestCriteriaFixture::QueryLoggedEntries(
    const std::string& sql_key, const std::string& logged_name) {
    try {
        if (m_db == nullptr) {
            LOG_MSG("No SQL database configured.");
            return std::nullopt;
        }

        const testing::TestInfo* test_info =
            testing::UnitTest::GetInstance()->current_test_info();
        if (test_info == nullptr) {
            LOG_MSG("No current test running.");
            return std::nullopt;
        }

        const std::string pattern = logged_name + "=%";
        const auto rows = m_db->Query(
            kCriteriaSqlFile.c_str(), sql_key.c_str(),
            {test_info->test_suite_name(), test_info->name(), pattern}, {});
        if (rows.empty()) {
            LOG_MSG("No rows returned for SQL key " + sql_key + " and logged name " + logged_name);
            return std::nullopt;
        }

        std::vector<std::string> values;
        const std::string prefix = logged_name + "=";
        for (const auto& row : rows) {
            if (row.empty()) { continue; }
            const std::string& entry = row.front();
            if (entry.rfind(prefix, 0) == 0) {
                values.push_back(entry.substr(prefix.size()));
            }
        }
        if (values.empty()) {
            LOG_MSG("No matching logged entries for " + logged_name);
            return std::nullopt;
        }
        return values;
    } catch (const std::exception& e) {
        LOG_MSG(std::string("Exception caught: ") + e.what());
        return std::nullopt;
    }
}

void GTestCriteriaFixture::SetUp()
{
    test_timer = std::chrono::high_resolution_clock::now();
}

void GTestCriteriaFixture::expect_performant(DurationStats stats, std::chrono::high_resolution_clock::duration test_duration, double threshold_multiplier)
{
    if (stats.count == 0) { return; }
    if (m_expect_performant_disabled) { return; }

    // Example performance expectation: test duration should not exceed mean +/- threshold_multiplier * stddev
    // if test duration is suddenly faster, it should still fail to assess performance.
    const double test_duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(test_duration).count();
    EXPECT_NEAR(test_duration_ms, stats.mean, threshold_multiplier * stats.stddev) << "Test duration exceeded performance threshold.";
}

void GTestCriteriaFixture::TearDown()
{
    /** 
     * We assume the difference between event listeners
     * (:testing::EmptyTestEventListener::OnTestEnd - ::testing::EmptyTestEventListener::OnTestStart)
     * is approximately the same difference between GTestCriteriaFixture::TearDown() - GTestCriteriaFixture::SetUp()
     * for a given suite.test.
     */
    auto test_duration = std::chrono::high_resolution_clock::now() - test_timer;
    const testing::TestInfo* test_info = testing::UnitTest::GetInstance()->current_test_info();
    const DurationStats test_stats = ComputeDurationStats(test_info->test_suite_name(), test_info->name());
    double threshold_multiplier = 3.0; // six-sigma rule
    expect_performant(test_stats, test_duration, threshold_multiplier);
}

void GTestCriteriaFixture::ParseDisableExpectArgument(int* argc, char* argv[]) {
    int remove_count = 0; // not really used, but interesting to know
    for (int i = 1; i < *argc; ++i) {
        const std::string argument(argv[i]);
        bool remove_argument = false;
        if (argument == kDisableExpectPerformantArgName) {
            m_expect_performant_disabled = true;
            remove_argument = true;
        } else if (argument == kDisableExpectIntegrityArgName) {
            m_expect_integrity_disabled = true;
            remove_argument = true;
        }
        if (remove_argument) {
            // shift elements left to overwrite/remove the argument from argv
            for (int j = i; j + 1 < *argc; ++j) {
                argv[j] = argv[j + 1];
            }
            argv[*argc - 1] = nullptr; // set the last element to nullptr after shifting
            --i; // adjust index to account for removed argument
            *argc = *argc - 1;
            remove_count++;
        }
    }
}

void GTestCriteriaFixture::log_internally(std::string msg, std::string file, int line_num) {
    // TODO define a proper logging mechanism
    // Currently, this just logs the message using SUCCEED().
    SUCCEED() << msg;
}

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

std::optional<std::vector<std::string>> GTestCriteriaFixture::QueryLoggedValueList(const std::string& logged_name) {
    try {
        if (m_db == nullptr) {
            ADD_FAILURE() << "GetValueList('" << logged_name
                          << "') returned nullopt: No SQL database is configured";
            return std::nullopt;
        }

        const testing::TestInfo* test_info =
            testing::UnitTest::GetInstance()->current_test_info();
        if (test_info == nullptr) {
            ADD_FAILURE() << "GetValueList('" << logged_name
                          << "') returned nullopt: GetValueList must be called while a test is running";
            return std::nullopt;
        }

        const std::string pattern = logged_name + "=%";
        const auto rows = m_db->Query(
            kCriteriaSqlFile.c_str(), "select_logged_value_list",
            {test_info->test_suite_name(), test_info->name(), pattern}, {});
        if (rows.empty()) {
            ADD_FAILURE() << "GetValueList('" << logged_name
                          << "') returned nullopt: No logged values found";
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
            ADD_FAILURE() << "GetValueList('" << logged_name
                          << "') returned nullopt: logged values are empty";
            return std::nullopt;
        }
        return values;
    } catch (const std::exception& error) {
        ADD_FAILURE() << "GetValueList('" << logged_name << "') returned nullopt: "
                      << error.what();
        return std::nullopt;
    }
}

std::optional<std::string> GTestCriteriaFixture::QueryLoggedValue(const std::string& logged_name) {
    try {
        if (m_db == nullptr) {
            ADD_FAILURE() << "GetValue('" << logged_name
                          << "') returned nullopt: No SQL database is configured";
            return std::nullopt;
        }

        const testing::TestInfo* test_info =
            testing::UnitTest::GetInstance()->current_test_info();
        if (test_info == nullptr) {
            ADD_FAILURE() << "GetValue('" << logged_name << "') returned nullopt: GetValue must be called while a test is running";
            return std::nullopt;
        }

        const std::string pattern = logged_name + "=%";
        const auto rows = m_db->Query(
            kCriteriaSqlFile.c_str(), "select_logged_value",
            {test_info->test_suite_name(), test_info->name(), pattern}, {});
        if (rows.empty() || rows.front().empty()) {
            ADD_FAILURE() << "GetValue('" << logged_name << "') returned nullopt: No logged value found";
            return std::nullopt;
        }

        const std::string prefix = logged_name + "=";
        const std::string value =
            rows.front().front().rfind(prefix, 0) == 0
                ? rows.front().front().substr(prefix.size())
                : std::string();
        if (value.empty()) {
            ADD_FAILURE() << "GetValue('" << logged_name << "') returned nullopt: logged value is empty";
            return std::nullopt;
        }
        return value;
    } catch (const std::exception& error) {
        ADD_FAILURE() << "GetValue('" << logged_name << "') returned nullopt: " << error.what();
        return std::nullopt;
    }
}

void GTestCriteriaFixture::SetUp()
{
    test_timer = std::chrono::high_resolution_clock::now();
}

void GTestCriteriaFixture::expect_performant(DurationStats stats, std::chrono::high_resolution_clock::duration test_duration)
{
    if (stats.count == 0) { return; }
    
    if (m_expect_performant_disabled) { return; }

    // Example performance expectation: test duration should not exceed mean + 2 * stddev
    const double test_duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(test_duration).count();
    EXPECT_NEAR(test_duration_ms, stats.mean, 2 * stats.stddev) << "Test duration exceeded performance threshold.";
}

void GTestCriteriaFixture::TearDown()
{
    auto test_duration = std::chrono::high_resolution_clock::now() - test_timer;
    const testing::TestInfo* test_info = testing::UnitTest::GetInstance()->current_test_info();
    const DurationStats test_stats = ComputeDurationStats(test_info->test_suite_name(), test_info->name());

    /** 
     * We assume the difference between event listeners
     * (:testing::EmptyTestEventListener::OnTestEnd - ::testing::EmptyTestEventListener::OnTestStart)
     * is approximately the same difference between GTestCriteriaFixture::TearDown() - GTestCriteriaFixture::SetUp()
     * for a given suite.test.
     */
    expect_performant(test_stats, test_duration);
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

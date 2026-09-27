#include <gtest/gtest.h>
#include <chrono>
#include <cstdint>
#include <string>
#include <filesystem>

#include "sqlite3.h"
#include "parse_argument.hpp"
#include "sql_backend.hpp"

#define GTEST_LOG_VALUE(name, value) LogValue(name, value, __LINE__ , __FILE__)

class GTestCriteriaFixture : public testing::Test
{
public:
    static void ConfigureSqlDb(std::optional<SqlOutput>);
protected:
    void SetUp() override;
    void TearDown() override;

    template <typename T>
    void LogValue(const std::string &name, T value, int line_num = 0, std::string file = "");

    template <typename T>
    T GetValue(const std::string &name);

    // Mean and (population) standard deviation of `end_timestamp -
    // start_timestamp` across all completed rows of a table, in the same
    // units as those timestamp columns (milliseconds).
    struct DurationStats
    {
        double mean = 0.0;
        double stddev = 0.0;
        std::int64_t count = 0;
    };

    // Computes DurationStats over rows in the "test" table for the test
    // named `test_name` within suite `suite_name`. Returns a
    // zero-initialized DurationStats if no SQL database is configured or no
    // completed rows match.
    static DurationStats ComputeDurationStats(const std::string& suite_name,
                                              const std::string& test_name);

    static void EXPECT_PERFORMANCE(DurationStats stats, std::chrono::high_resolution_clock::duration test_duration);

    std::chrono::high_resolution_clock::time_point test_timer;
    static sqlite3* m_db;
};

template <typename T>
void GTestCriteriaFixture::LogValue(const std::string &name, T value, int line_num, std::string file)
{
    testing::Message msg;
    msg << name << "=" << value;
    testing::TestPartResult::Type type = testing::TestPartResult::Type::kSuccess;
    testing::internal::AssertHelper ah {type, file, line_num, ""};
    // =op calls the logging of test result part event
    ah = msg;
}

template <typename T>
T GTestCriteriaFixture::GetValue(const std::string &name)
{
    T value{};
    const char* sql =
        "SELECT message FROM test JOIN suite ON test.suite_id = suite.id "
    "WHERE suite.name = ? AND test.name = ?;";

    sqlite3_stmt* statement = nullptr;
    if (sqlite3_prepare_v2(m_db, sql, -1, &statement, nullptr) != SQLITE_OK) {
        return value;
    }
    return value;
}

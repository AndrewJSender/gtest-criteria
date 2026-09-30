// Copyright (c) 2026 Andrew J. Sender.
// All rights reserved.

#include <gtest/gtest.h>
#include <chrono>
#include <cstdint>
#include <sstream>
#include <stdexcept>
#include <string>

#include "parse_argument.hpp"
#include "sql_backend.hpp"

#define GTEST_LOG_VALUE(name, value) LogValue(name, value, __LINE__ , __FILE__)

class GTestCriteriaFixture : public testing::Test
{
public:
    static void ConfigureSqlDb(std::optional<SqlOutput>);
    static void ParseDisableExpectArgument(int* argc, char* argv[]);
protected:
    void SetUp() override;
    void TearDown() override;

    static bool IsExpectIntegrityDisabled() { return m_expect_integrity_disabled; }
    
    template <typename T>
    void LogValue(const std::string &logged_name, T value, int line_num = 0, std::string file = "");

    template <typename T>
    T GetValue(const std::string &logged_name);
private:
    static std::string QueryLoggedValue(const std::string& logged_name);

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

    static void EXPECT_PERFORMANT(DurationStats stats, std::chrono::high_resolution_clock::duration test_duration);

    std::chrono::high_resolution_clock::time_point test_timer;
    static std::unique_ptr<testing::SqlBackend> m_db;
    static bool m_expect_performant_disabled;
    static bool m_expect_integrity_disabled;
};

template <typename T>
void GTestCriteriaFixture::LogValue(const std::string &logged_name, T value, int line_num, std::string file)
{
    testing::Message msg;
    msg << logged_name << "=" << value;
    testing::TestPartResult::Type type = testing::TestPartResult::Type::kSuccess;
    testing::internal::AssertHelper ah {type, file, line_num, ""};
    // =op calls the logging of test result part event
    ah = msg;
}

template <typename T>
T GTestCriteriaFixture::GetValue(const std::string &logged_name)
{
    std::istringstream input(QueryLoggedValue(logged_name));
    T value{};
    if (!(input >> value) || (input >> std::ws && !input.eof())) {
        throw std::runtime_error("Unable to parse logged value '" + logged_name + "'");
    }
    return value;
}

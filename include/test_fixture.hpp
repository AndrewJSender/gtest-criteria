// Copyright (c) 2026 Andrew J. Sender.
// All rights reserved.

#ifndef GTEST_CRITERIA_FIXTURE_HPP
#define GTEST_CRITERIA_FIXTURE_HPP

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <chrono>
#include <cstdint>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

#include "parse_argument.hpp"
#include "sql_backend.hpp"

#define LOG_MSG(msg) log_internally(msg, __FILE__, __LINE__)

#define EXPECT_THAT_AND_LOG(result, matcher_lambda)                                      \
    expect_that_and_log(result, #result,                                                 \
                        IsExpectIntegrityDisabled()                                      \
                            ? decltype((matcher_lambda)(#result)){}                       \
                            : (matcher_lambda)(#result),                                  \
                        __FILE__, __LINE__)

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
    void LogValue(const std::string &logged_name, T value, testing::TestPartResult::Type type, std::string file, int line_num);

    template <typename T>
    std::optional<std::vector<T>> GetValueList(const std::string &logged_name);

    template <typename T>
    void expect_that_and_log(T result, const std::string &result_name, std::optional<::testing::Matcher<T>> gmock_matcher, std::string file_name, int line_num);

    std::optional<float> GetTestFailureRate();
private:
    // Runs `sql_key` against the current test's logged
    // "name=value" entries matching `logged_name`, and returns the values
    // with the "name=" prefix stripped. Returns std::nullopt if no database
    // is configured, no test is running, or no matching values are found.
    /**
     * @brief Queries the SQL database for logged entries matching the given key and name.
     * 
     * @param sql_key The SQL key to query.
     * @param logged_name The name of the logged entries to retrieve.
     * @return std::optional<std::vector<std::string>> A vector of matching logged values with the "name=" prefix stripped, or std::nullopt if no matching entries are found.
     */
    std::optional<std::vector<std::string>> QueryLoggedEntries(const std::string& sql_key, const std::string& logged_name);

    // Parses `raw` as a single value of type T, requiring the entire
    // string to be consumed. Returns std::nullopt if parsing fails.
    template <typename T>
    static std::optional<T> ParseLoggedValue(const std::string& raw);

    // Queries entries via `sql_key` (see QueryLoggedEntries) and parses each one as a T.
    // Returns std::nullopt if the query finds nothing or any entry fails to
    // parse.
    template <typename T>
    std::optional<std::vector<T>> GetValues(const std::string& sql_key, const std::string& logged_name);

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

    /**
     * @brief Checks if the test duration meets the performance expectations based on the provided statistics.
     * 
     * @param stats The duration statistics to compare against.
     * @param test_duration The actual duration of the test.
     * @param threshold_multiplier The multiplier for the standard deviation to define the acceptable performance range.
     * @param min_difference The minimum difference in milliseconds to consider a performance deviation significant.
     */
    static void expect_performant(DurationStats stats, std::chrono::high_resolution_clock::duration test_duration, double threshold_multiplier, double min_difference);

    /**
     * @brief Logs a message internally, typically for test results.
     * 
     * @param msg The message to log.
     * @param file_name The name of the source file where the log is generated.
     * @param line_num The line number in the source file where the log is generated.
     */
    static void log_internally(std::string msg, std::string file_name, int line_num);
    
    std::chrono::high_resolution_clock::time_point test_timer;
    static std::unique_ptr<testing::SqlBackend> m_db;
    static bool m_expect_performant_disabled;
    static bool m_expect_integrity_disabled;
};

#include "test_fixture.tpp"

#endif // GTEST_CRITERIA_FIXTURE_HPP

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

#define EXPECT_THAT_AND_LOG(result, matcher) \
std::string name = #result ; \
auto gmock_matcher = matcher(name); \
TestPartResultType result_type = TestPartResultType::kSuccess; \
if (!IsExpectIntegrityDisabled() && gmock_matcher.has_value()) { \
    EXPECT_THAT(result, *gmock_matcher); \
    if (!::testing::Matcher<decltype(result)>(*gmock_matcher).Matches(result)) { \
        result_type = TestPartResultType::kNonFatalFailure; \
    } \
} \
LogValue(name, result, result_type, __LINE__ , __FILE__);

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
    void LogValue(const std::string &logged_name, T value, testing::TestPartResult::Type type = testing::TestPartResult::Type::kSuccess, int line_num = 0, std::string file = "");

    template <typename T>
    std::optional<T> GetValue(const std::string &logged_name);

    template <typename T>
    std::optional<std::vector<T>> GetValueList(const std::string &logged_name);
private:
    // Runs `sql_key` (one of "select_logged_value" or
    // "select_logged_value_list") against the current test's logged
    // "name=value" entries matching `logged_name`, and returns the values
    // with the "name=" prefix stripped. Returns std::nullopt if no database
    // is configured, no test is running, or no matching values are found.
    static std::optional<std::vector<std::string>> QueryLoggedEntries(
        const std::string& sql_key, const std::string& logged_name);

    // Parses `raw` as a single value of type T, requiring the entire
    // string to be consumed. Returns std::nullopt if parsing fails.
    template <typename T>
    static std::optional<T> ParseLoggedValue(const std::string& raw);

    // Shared implementation for GetValue(List): queries entries via
    // `sql_key` (see QueryLoggedEntries) and parses each one as a T.
    // Returns std::nullopt if the query finds nothing or any entry fails to
    // parse.
    template <typename T>
    static std::optional<std::vector<T>> GetValues(const std::string& sql_key, const std::string& logged_name);

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

    static void expect_performant(DurationStats stats, std::chrono::high_resolution_clock::duration test_duration);

    std::chrono::high_resolution_clock::time_point test_timer;
    static std::unique_ptr<testing::SqlBackend> m_db;
    static bool m_expect_performant_disabled;
    static bool m_expect_integrity_disabled;
};

template <typename T>
void GTestCriteriaFixture::LogValue(const std::string &logged_name, T value, testing::TestPartResult::Type type, int line_num, std::string file)
{
    testing::Message msg;
    msg << logged_name << "=" << value;
    testing::internal::AssertHelper ah {type, file.c_str(), line_num, ""};
    // =op calls the logging of test result part event
    ah = msg;
}

template <typename T>
std::optional<T> GTestCriteriaFixture::ParseLoggedValue(const std::string& raw)
{
    std::istringstream input(raw);
    T value{};
    if (!(input >> value) || (input >> std::ws, !input.eof())) {
        std::ostringstream msg; msg << "Failed to parse logged value: " << raw;
        return std::nullopt;
    }
    return value;
}

template <typename T>
std::optional<std::vector<T>> GTestCriteriaFixture::GetValues(const std::string& sql_key, const std::string& logged_name)
{
    try {
        auto logged_values = QueryLoggedEntries(sql_key, logged_name);
        if (!logged_values.has_value()) {
            std::ostringstream msg; msg << "No logged values returned for SQL key " << sql_key << " and logged name " << logged_name;
            return std::nullopt;
        }
        std::vector<T> values;
        for (const auto& value : *logged_values) {
            auto parsed_value = ParseLoggedValue<T>(value);
            if (!parsed_value.has_value()) {
                std::ostringstream msg; msg << "Failed to parse logged value: " << value;
                return std::nullopt;
            }
            values.push_back(*parsed_value);
        }
        return values;
    } catch (const std::exception& e) {
        std::ostringstream msg; msg << e.what();
        return std::nullopt;
    }
}

template <typename T>
std::optional<T> GTestCriteriaFixture::GetValue(const std::string &logged_name)
{
    auto values = GetValues<T>("select_logged_value", logged_name);
    if (!values.has_value() || values->empty()) {
        std::ostringstream msg; msg << "No values returned for logged name " << logged_name;
        return std::nullopt;
    }
    return values->front();
}

template <typename T>
std::optional<std::vector<T>> GTestCriteriaFixture::GetValueList(const std::string &logged_name)
{
    return GetValues<T>("select_logged_value_list", logged_name);
}

#endif // GTEST_CRITERIA_FIXTURE_HPP

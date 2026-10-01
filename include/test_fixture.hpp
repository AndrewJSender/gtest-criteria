// Copyright (c) 2026 Andrew J. Sender.
// All rights reserved.

#ifndef GTEST_CRITERIA_FIXTURE_HPP
#define GTEST_CRITERIA_FIXTURE_HPP

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
if (!IsExpectIntegrityDisabled() && !gmock_matcher) { \
    EXPECT_THAT(result, gmock_matcher); \
    if (!::testing::Matcher<decltype(result)>(gmock_matcher).Matches(result)) { \
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
    static std::optional<std::string> QueryLoggedValue(const std::string& logged_name);
    static std::optional<std::vector<std::string>> QueryLoggedValueList(const std::string& logged_name);

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
std::optional<T> GTestCriteriaFixture::GetValue(const std::string &logged_name)
{
    try {
        auto logged_value = QueryLoggedValue(logged_name);
        if (!logged_value.has_value()) {
            return std::nullopt;
        }
        std::istringstream input(*logged_value);
        T value{};
        if (!(input >> value) || (input >> std::ws, !input.eof())) {
            ADD_FAILURE() << "GetValue('" << logged_name
                          << "') returned nullopt: unable to parse logged value";
            return std::nullopt;
        }
        return value;
    } catch (const std::exception& error) {
        ADD_FAILURE() << "GetValue('" << logged_name << "') returned nullopt: "
                      << error.what();
        return std::nullopt;
    }
}

template <typename T>
std::optional<std::vector<T>> GTestCriteriaFixture::GetValueList(const std::string &logged_name)
{
    try {
        auto logged_values = QueryLoggedValueList(logged_name);
        if (!logged_values.has_value()) {
            return std::nullopt;
        }
        std::vector<T> values;
        for (const auto& value : *logged_values) {
            std::istringstream input(value);
            T parsed_value{};
            if (!(input >> parsed_value) || (input >> std::ws, !input.eof())) {
                ADD_FAILURE() << "GetValueList('" << logged_name
                              << "') returned nullopt: unable to parse logged value '"
                              << value << "'";
                return std::nullopt;
            }
            values.push_back(parsed_value);
        }
        return values;
    } catch (const std::exception& error) {
        ADD_FAILURE() << "GetValueList('" << logged_name << "') returned nullopt: "
                      << error.what();
        return std::nullopt;
    }
}

#endif // GTEST_CRITERIA_FIXTURE_HPP

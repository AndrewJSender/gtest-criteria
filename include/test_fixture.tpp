// Copyright (c) 2026 Andrew J. Sender.
// All rights reserved.

template <typename T>
void GTestCriteriaFixture::LogValue(const std::string &logged_name, T value, TestPartResultType type, std::string file, int line_num)
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
        LOG_MSG("Failed to parse logged value: " + raw);
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
            LOG_MSG("No logged values returned for SQL key " + sql_key + " and logged name " + logged_name);
            return std::nullopt;
        }
        std::vector<T> values;
        for (const auto& value : *logged_values) {
            auto parsed_value = ParseLoggedValue<T>(value);
            if (!parsed_value.has_value()) {

                LOG_MSG("Failed to parse logged value: " + value);
                return std::nullopt;
            }
            values.push_back(*parsed_value);
        }
        return values;
    } catch (const std::exception& e) {
        LOG_MSG("Exception caught: " + std::string(e.what()));
        return std::nullopt;
    }
}

template <typename T>
std::optional<std::vector<T>> GTestCriteriaFixture::GetValueList(const std::string &logged_name)
{
    return GetValues<T>("select_logged_value_list", logged_name);
}

template <typename ResultType, typename MatcherFactory>
GTestCriteriaFixture::TestPartResultType GTestCriteriaFixture::run_matcher_lambda_and_expect_that(ResultType result, const std::string &result_name, MatcherFactory matcher_lambda) {
    std::optional<testing::Matcher<ResultType>> gmock_matcher = matcher_lambda(result_name);
    if (!gmock_matcher.has_value()) {
        return GTestCriteriaFixture::TestPartResultType::kSuccess;
    }
    EXPECT_THAT(result, *gmock_matcher);
    if (testing::Matcher<ResultType>(*gmock_matcher).Matches(result)) {
        return GTestCriteriaFixture::TestPartResultType::kSuccess;
    } else {
        return GTestCriteriaFixture::TestPartResultType::kNonFatalFailure;
    }
}

template <typename ResultType, typename MatcherFactory>
void GTestCriteriaFixture::expect_that_and_log(ResultType result, const std::string &result_name, MatcherFactory matcher_lambda, std::string file_name, int line_num) {
    // The result type initiates to success, which is inaccurate.
    // However, GTest evaluates the overall test result by its parts,
    // If any part is non-success, the overall test result will reflect the failure or skip.
    TestPartResultType result_type = TestPartResultType::kSuccess;
    if (!m_expect_integrity_disabled) {
        result_type = run_matcher_lambda_and_expect_that(result, result_name, matcher_lambda);
    }
    LogValue(result_name, result, result_type , file_name, line_num);
}

template <typename ResultType, typename MatcherFactory>
void GTestCriteriaFixture::expect_that_and_stable(ResultType result, MatcherFactory matcher_lambda, float stable_threshold, std::string file_name, int line_num) {
    if (m_expect_stable_disabled) { return; }
    ASSERT_LT(stable_threshold, 1.0) << "Stable threshold must be less than 1.0";
    ASSERT_GT(stable_threshold, 0.0) << "Stable threshold must be greater than 0.0";

    std::optional<testing::Matcher<ResultType>> gmock_matcher = matcher_lambda();
    if (!gmock_matcher.has_value()) {
        return;
    }
    if (testing::Matcher<ResultType>(*gmock_matcher).Matches(result)) {
        EXPECT_THAT(result, *gmock_matcher);
        return;
    }

    // non-fatal failure handling based on historical failure rate
    auto historical_failure_rate = GetTestFailureRate();
    if (!historical_failure_rate.has_value()) {
        EXPECT_THAT(result, *gmock_matcher);
        return;
    }
    if (historical_failure_rate.value() < stable_threshold) {
        std::string msg = "Test is stable: historical failure rate " + std::to_string(historical_failure_rate.value()) + " is below the stable threshold " + std::to_string(stable_threshold) + ".";
        GTEST_SKIP() << msg;
    } else {
        EXPECT_THAT(result, *gmock_matcher);
    }
}

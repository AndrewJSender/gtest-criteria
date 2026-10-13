// Copyright (c) 2026 Andrew J. Sender.
// All rights reserved.

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <chrono>
#include <thread>
#include <random>
#include <optional>

#include "parse_argument.hpp"
#include "register_listener.hpp"
#include "sql_test_event_listner.hpp"
#include "test_fixture.hpp"

class TestExampleFixture : public GTestCriteriaFixture {
protected:
    // TODO: add an argument parser --exert_failure
    bool m_exert_failure = false;

    float test_normal_dist(float mean, float stdev) {
        std::random_device rd{};
        std::mt19937 gen{rd()};

        // Values near the mean are the most likely. Standard deviation
        // affects the dispersion of generated values from the mean.
        std::normal_distribution d{mean, stdev};

        // Draw a sample from the normal distribution and round it to an integer.
        auto random_float = [&d, &gen]{ return d(gen); };
        return random_float();
    }

    std::pair<float, float> calculate_mean_and_stdev(std::vector<float> values_list) {
        float sum_linear = 0.0f;
        float sum_sq = 0.0f;
        for (float value : values_list) {
            sum_linear += value;
            sum_sq += value * value;
        }
        float mean = sum_linear / values_list.size();
        float stdev = std::sqrt((sum_sq / values_list.size()) - (mean * mean));

        return {mean, stdev};
    }
};

TEST_F(TestExampleFixture, Performance) {
    int input_value = 1;
    if (m_exert_failure) {
        input_value = 3;
    }
    std::this_thread::sleep_for(std::chrono::seconds(input_value));
    // Performance evaluated in TestExampleFixture::TearDown
}

TEST_F(TestExampleFixture, Integrity) {
    float input_mean = 0.0f;
    float stdev = 1.0f;
    float result = test_normal_dist(input_mean, stdev);
    if (m_exert_failure) {
        input_mean = 10.0f;
    }
    auto matcher = [this, c_stdev = stdev](std::string name) -> std::optional<testing::Matcher<float>> {
        auto values_list = GetValueList<float>(name);
        if (!values_list.has_value()) {
            return std::nullopt;
        }
        float expected_avg, expected_stdev;
        std::tie(expected_avg, expected_stdev) = calculate_mean_and_stdev(*values_list);
        return testing::FloatNear(expected_avg, 3 * c_stdev);
    };
    EXPECT_THAT_AND_LOG(result, matcher);
}

TEST_F(TestExampleFixture, Stability) {
    float failure_rate = 0.05;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dis(0, 1.0);
    float uniform_rd_number = dis(gen);
    EXPECT_LT(failure_rate, uniform_rd_number);
    double stable_threshold = 0.05;
    expect_stable(stable_threshold);
}

int main(int argc, char* argv[]) {
    auto sql_output = ParseGtestSqlOutputArgument(&argc, argv);
    GTestCriteriaFixture::ParseDisableExpectArguments(&argc, argv);
    testing::InitGoogleTest(&argc, argv);
    RegisterSqlTestEventListenerIfRequested(sql_output);
    GTestCriteriaFixture::ConfigureSqlDb(sql_output);
    return RUN_ALL_TESTS();
}

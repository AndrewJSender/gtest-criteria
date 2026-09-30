// Copyright (c) 2026 Andrew J. Sender.
// All rights reserved.

#include <gtest/gtest.h>

#include <chrono>
#include <thread>

#include "parse_argument.hpp"
#include "register_listener.hpp"
#include "sql_test_event_listner.hpp"
#include "test_fixture.hpp"

class MyTestFixture : public GTestCriteriaFixture {
protected:
    int test_function(int input_value) {
        // performing some operation before returning the result
        std::this_thread::sleep_for(std::chrono::seconds(input_value));
        return input_value * 2;
    }
};

TEST_F(MyTestFixture, Test1) {
    int input_value = 5;
    int result = test_function(input_value);
    std::string name = "result";

    GTEST_LOG_VALUE(name, result);
    if (!IsExpectIntegrityDisabled()) {
        int expected_result = GetValue<int>(name);
        EXPECT_EQ(result, expected_result);
    }
}

int main(int argc, char* argv[]) {
    auto sql_output = ParseGtestSqlOutputArgument(&argc, argv);
    GTestCriteriaFixture::ParseDisableExpectArgument(&argc, argv);
    testing::InitGoogleTest(&argc, argv);
    RegisterSqlTestEventListenerIfRequested(sql_output);
    GTestCriteriaFixture::ConfigureSqlDb(sql_output);
    return RUN_ALL_TESTS();
}

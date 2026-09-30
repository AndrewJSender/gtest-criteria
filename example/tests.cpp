


#include "test_fixture.hpp"

class MyTestFixture : public GTestCriteriaFixture {
protected:
    int test_function(int input_value) {
        // performing some operation before returning the result
        sleep(input_value);
        return input_value * 2;
    }
};

TEST_F(MyTestFixture, Test1) {
    int input_value = 1;
    int result = test_function(input_value);
    std::string name = "result";

    GTEST_LOG_VALUE(name, result);
    int expected_result = GetValue<int>(name);
    // EXPECT_EQ(result, expected_result);
}

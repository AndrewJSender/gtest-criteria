
#include "test_fixture.hpp"

void GTestCriteriaFixture::SetUp() {
    test_timer = std::chrono::high_resolution_clock::now();
}

void GTestCriteriaFixture::TearDown() {
    auto test_duration = std::chrono::high_resolution_clock::now() - test_timer;
    
    // TODO Retrieve average test duration of previous runs from
    // some external source and calculate the average test duration.
}

template <typename T>
void GTestCriteriaFixture::LogValue(const std::string& name, T value) {
    GTEST_SUCCEED() << name << ":" << value << std::endl;
}

template <typename T>
T GTestCriteriaFixture::GetValue(const std::string& name) {
    T value;
    // TODO Retrieve the value from some external source based on the name.
    return value;
}
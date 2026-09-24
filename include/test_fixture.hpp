#include <gtest/gtest.h>
#include <chrono>
#include <thread>

class GTestCriteriaFixture : public ::testing::Test {
protected:
    void SetUp() override;
    void TearDown() override;

    template <typename T>
    void LogValue(const std::string& name, T value);

    template <typename T>
    T GetValue(const std::string& name, T value);

    std::chrono::high_resolution_clock::time_point test_timer;
};
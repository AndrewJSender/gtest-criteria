#include <gtest/gtest.h>
#include <chrono>
#include <string>
#include <filesystem>

#include "sqlite3.h"
#include "parse_argument.hpp"
#include "sql_backend.hpp"

class GTestCriteriaFixture : public testing::Test
{
public:
    static void ConfigureSqlDb(std::optional<SqlOutput>);
protected:
    void SetUp() override;
    void TearDown() override;

    template <typename T>
    void LogValue(const std::string &name, T value);

    template <typename T>
    T GetValue(const std::string &name);

    std::chrono::high_resolution_clock::time_point test_timer;
//    static std::unique_ptr<testing::SqlBackend> m_backend;
    static sqlite3* m_db;
};

template <typename T>
void GTestCriteriaFixture::LogValue(const std::string &name, T value)
{
    GTEST_SUCCEED() << name << ":" << value << std::endl;
}

template <typename T>
T GTestCriteriaFixture::GetValue(const std::string &name)
{
    T value{};
    // TODO Retrieve the value from some external source based on the name.
    return value;
}

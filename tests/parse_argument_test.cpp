// Copyright (c) 2026 Andrew J. Sender.
// All rights reserved.

#include "parse_argument.hpp"
#include "test_fixture.hpp"

#include <gtest/gtest.h>

#include <array>
#include <string>

namespace {

TEST(ParseArgumentTest, ParsesAndRemovesSQLiteOutputArgument) {
    std::array<std::string, 3> storage{"test", "--gtest_output=sqlite:results.db",
                                       "--gtest_repeat=2"};
    std::array<char*, 4> argv{
        storage[0].data(), storage[1].data(), storage[2].data(), nullptr};
    int argc = 3;

    const auto output = ParseGtestSqlOutputArgument(&argc, argv.data());

    ASSERT_TRUE(output.has_value());
    EXPECT_EQ(output->type, SqlOutputType::SQLite);
    EXPECT_EQ(output->connection, "results.db");
    EXPECT_EQ(argc, 2);
    EXPECT_STREQ(argv[1], "--gtest_repeat=2");
    EXPECT_EQ(argv[2], nullptr);
}

TEST(ParseArgumentTest, RemovesCriteriaDisableArguments) {
    std::array<std::string, 5> storage{
        "test", "--criteria_disable_expect_performant",
        "--criteria_disable_expect_integrity", "--criteria_disable_expect_stable",
        "--gtest_repeat=2"};
    std::array<char*, 6> argv{storage[0].data(), storage[1].data(),
                              storage[2].data(), storage[3].data(),
                              storage[4].data(), nullptr};
    int argc = 5;

    GTestCriteriaFixture::ParseDisableExpectArguments(&argc, argv.data());

    EXPECT_EQ(argc, 2);
    EXPECT_STREQ(argv[1], "--gtest_repeat=2");
    EXPECT_EQ(argv[2], nullptr);
}

#if !GTEST_CRITERIA_HAS_POSTGRESQL
TEST(PostgreSqlOptionTest, ReportsDisabledBackendExplicitly) {
    const SqlOutput output{
        SqlOutputType::PostgreSQL,
        "postgresql://localhost/gtest_criteria",
    };

    EXPECT_THROW(GTestCriteriaFixture::ConfigureSqlDb(output),
                 std::runtime_error);
}
#endif

}  // namespace

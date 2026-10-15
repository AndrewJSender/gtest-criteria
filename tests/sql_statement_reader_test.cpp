// Copyright (c) 2026 Andrew J. Sender.
// All rights reserved.

#include "sql_statement_reader.hpp"

#include <gtest/gtest.h>

#include <string>

TEST(SqlStatementReaderTest, FallsBackToEmbeddedSqlResources) {
    const std::string statement = testing::ReadSqlStatement(
        "sqlite", "unavailable/criteria.sql", "select_test_durations");

    EXPECT_NE(statement.find("SELECT AVG(duration)"), std::string::npos);
    EXPECT_NE(statement.find("LIMIT ?"), std::string::npos);
}

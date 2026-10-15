// Copyright (c) 2026 Andrew J. Sender.
// All rights reserved.

#include "sqlite_backend.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <string>

namespace {

class TemporaryDatabase {
 public:
    TemporaryDatabase()
        : path_(std::filesystem::temp_directory_path() /
                ("gtest-criteria-" +
                 std::to_string(
                     std::chrono::steady_clock::now().time_since_epoch().count()) +
                 ".db")) {}

    ~TemporaryDatabase() {
        std::error_code error;
        std::filesystem::remove(path_, error);
    }

    const std::filesystem::path& path() const { return path_; }

 private:
    std::filesystem::path path_;
};

TEST(SqliteBackendTest, InitializesSchemaAndPersistsCompletedTest) {
    TemporaryDatabase database;
    testing::SqliteBackend backend(database.path());

    const auto program_id = backend.InsertRow(
        "program.sql", "program_insert", {"running"}, {100});
    const auto suite_id = backend.InsertRow(
        "suite.sql", "suite_insert", {"Suite", "running"},
        {110, program_id});
    const auto test_id = backend.InsertRow(
        "test.sql", "test_insert", {"Test", "running"}, {120, suite_id});
    backend.Execute("test.sql", "test_update", {"passed"}, {130, test_id});

    const auto rows = backend.Query(
        GTEST_CRITERIA_TEST_SQL_DIR "/criteria.sql",
        "select_test_durations", {"Suite", "Test"}, {100});

    ASSERT_EQ(rows.size(), 1U);
    ASSERT_EQ(rows.front().size(), 3U);
    EXPECT_EQ(rows.front()[0], "10.0");
    EXPECT_EQ(rows.front()[2], "1");
}

TEST(SqliteBackendTest, LimitsStabilityHistoryToMostRecentRows) {
    TemporaryDatabase database;
    testing::SqliteBackend backend(database.path());

    const auto program_id = backend.InsertRow(
        "program.sql", "program_insert", {"running"}, {100});
    const auto suite_id = backend.InsertRow(
        "suite.sql", "suite_insert", {"Suite", "running"},
        {110, program_id});

    for (int index = 0; index < 4; ++index) {
        const auto test_id = backend.InsertRow(
            "test.sql", "test_insert", {"Test", "running"},
            {120 + index, suite_id});
        backend.Execute("test.sql", "test_update",
                        {index == 0 ? "failed" : "passed"},
                        {130 + index, test_id});
    }

    const auto rows = backend.Query(
        GTEST_CRITERIA_TEST_SQL_DIR "/criteria.sql",
        "select_test_failure_or_skip_rate", {"Suite", "Test"}, {3});

    ASSERT_EQ(rows.size(), 1U);
    ASSERT_EQ(rows.front().size(), 2U);
    EXPECT_EQ(rows.front()[0], "0.0");
    EXPECT_EQ(rows.front()[1], "3");
}

}  // namespace

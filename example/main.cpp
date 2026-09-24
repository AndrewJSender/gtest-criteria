
#include "sql_test_event_listner.hpp"
#include <gtest/gtest.h>
#include "parse_argument.hpp"

int main(int argc, char* argv[]) {
    auto sql_output = ParseGtestSqlOutputArgument(&argc, argv);
    testing::InitGoogleTest(&argc, argv);
    if (sql_output.has_value()) {
        testing::TestEventListeners& listeners =
            testing::UnitTest::GetInstance()->listeners();
          auto* sql_listener = sql_output->type == SqlOutputType::SQLite
                                   ? new testing::SqlTestEventListener(
                                         std::filesystem::path(
                                             sql_output->connection))
                                   : new testing::SqlTestEventListener(
                                         testing::SqlTestEventListener::
                                             PostgreSqlTag{},
                                         sql_output->connection);
          listeners.Append(sql_listener);
    }
    return RUN_ALL_TESTS();
}

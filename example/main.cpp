

#include <gtest/gtest.h>
#include "parse_argument.hpp"
#include "register_listener.hpp"
#include "sql_test_event_listner.hpp"
#include "test_fixture.hpp"

int main(int argc, char* argv[]) {
    auto sql_output = ParseGtestSqlOutputArgument(&argc, argv);
    testing::InitGoogleTest(&argc, argv);
    RegisterSqlTestEventListenerIfRequested(sql_output);
    GTestCriteriaFixture::ConfigureSqlDb(sql_output);
    return RUN_ALL_TESTS();
}

add_executable(gtest_criteria_tests
  criteria_policy_test.cpp
  parse_argument_test.cpp
  sqlite_backend_test.cpp
  sql_statement_reader_test.cpp
  test_main.cpp
)
target_link_libraries(gtest_criteria_tests PRIVATE GTestCriteria::gtest_criteria)
target_compile_definitions(gtest_criteria_tests PRIVATE
  GTEST_CRITERIA_TEST_SQL_DIR="${PROJECT_SOURCE_DIR}/sql/sqlite"
)

if(CMAKE_GENERATOR STREQUAL "Xcode")
  add_test(NAME gtest_criteria_tests COMMAND gtest_criteria_tests)
else()
  include(GoogleTest)
  gtest_discover_tests(gtest_criteria_tests)
endif()

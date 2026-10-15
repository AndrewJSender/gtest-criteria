cmake_minimum_required(VERSION 3.16)

project(
  gtest_criteria
  VERSION 0.1.0
  DESCRIPTION "History-aware integrity, stability, and performance checks for GoogleTest"
  LANGUAGES CXX
)

include(CMakePackageConfigHelpers)
include(CTest)
include(GNUInstallDirs)

option(GTEST_CRITERIA_BUILD_EXAMPLE "Build the example executable" ON)
option(GTEST_CRITERIA_USE_BUNDLED_GTEST "Use the bundled GoogleTest submodule" ON)
option(GTEST_CRITERIA_WITH_POSTGRESQL "Build PostgreSQL backend support" OFF)

set(GTEST_CRITERIA_SQL_OUTPUT_ROOT
    "${CMAKE_CURRENT_SOURCE_DIR}/submodules/gtest_output-sql")
set(GTEST_CRITERIA_GTEST_SOURCE_DIR
    "${GTEST_CRITERIA_SQL_OUTPUT_ROOT}/submodules/googletest")

if(GTEST_CRITERIA_USE_BUNDLED_GTEST)
  if(NOT EXISTS "${GTEST_CRITERIA_GTEST_SOURCE_DIR}/CMakeLists.txt")
    message(FATAL_ERROR
      "GoogleTest is missing. Run: git submodule update --init --recursive")
  endif()
  set(INSTALL_GTEST ON CACHE BOOL "" FORCE)
  add_subdirectory(
    "${GTEST_CRITERIA_GTEST_SOURCE_DIR}"
    "${CMAKE_CURRENT_BINARY_DIR}/googletest"
  )
  set(GTEST_CRITERIA_GMOCK_TARGET GTest::gmock)
else()
  find_package(GTest CONFIG REQUIRED)
  set(GTEST_CRITERIA_GMOCK_TARGET GTest::gmock)
endif()

find_package(SQLite3 REQUIRED)
if(NOT TARGET SQLite3::SQLite3)
  add_library(SQLite3::SQLite3 INTERFACE IMPORTED)
  set_property(TARGET SQLite3::SQLite3 PROPERTY
    INTERFACE_LINK_LIBRARIES SQLite::SQLite3)
endif()

file(READ "${GTEST_CRITERIA_SQL_OUTPUT_ROOT}/sql/environment.sql"
  GTEST_CRITERIA_SQL_ENVIRONMENT)
file(READ "${GTEST_CRITERIA_SQL_OUTPUT_ROOT}/sql/iteration.sql"
  GTEST_CRITERIA_SQL_ITERATION)
file(READ "${GTEST_CRITERIA_SQL_OUTPUT_ROOT}/sql/program.sql"
  GTEST_CRITERIA_SQL_PROGRAM)
file(READ "${GTEST_CRITERIA_SQL_OUTPUT_ROOT}/sql/suite.sql"
  GTEST_CRITERIA_SQL_SUITE)
file(READ "${GTEST_CRITERIA_SQL_OUTPUT_ROOT}/sql/test.sql"
  GTEST_CRITERIA_SQL_TEST)
file(READ "${GTEST_CRITERIA_SQL_OUTPUT_ROOT}/sql/test_result_part.sql"
  GTEST_CRITERIA_SQL_TEST_RESULT_PART)
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/sql/sqlite/criteria.sql"
  GTEST_CRITERIA_SQL_CRITERIA)
configure_file(
  cmake/embedded_sql.hpp.in
  "${CMAKE_CURRENT_BINARY_DIR}/generated/embedded_sql.hpp"
  @ONLY
)

set(GTEST_CRITERIA_SOURCES
  src/criteria_policy.cpp
  src/sql_statement_reader.cpp
  src/test_fixture.cpp
  "${GTEST_CRITERIA_SQL_OUTPUT_ROOT}/src/parse_argument.cpp"
  "${GTEST_CRITERIA_SQL_OUTPUT_ROOT}/src/register_listener.cpp"
  "${GTEST_CRITERIA_SQL_OUTPUT_ROOT}/src/sql_test_event_listner.cpp"
  "${GTEST_CRITERIA_SQL_OUTPUT_ROOT}/src/sqlite_backend.cpp"
)

if(GTEST_CRITERIA_WITH_POSTGRESQL)
  find_package(PostgreSQL REQUIRED)
  list(APPEND GTEST_CRITERIA_SOURCES
    "${GTEST_CRITERIA_SQL_OUTPUT_ROOT}/src/postgresql_backend.cpp")
else()
  list(APPEND GTEST_CRITERIA_SOURCES src/postgresql_backend_stub.cpp)
endif()

add_library(gtest_criteria ${GTEST_CRITERIA_SOURCES})
add_library(GTestCriteria::gtest_criteria ALIAS gtest_criteria)
set_target_properties(gtest_criteria PROPERTIES
  EXPORT_NAME gtest_criteria
  VERSION "${PROJECT_VERSION}"
  SOVERSION "${PROJECT_VERSION_MAJOR}"
)
target_compile_features(gtest_criteria PUBLIC cxx_std_17)
target_include_directories(gtest_criteria
  PRIVATE
    "${CMAKE_CURRENT_BINARY_DIR}/generated"
  PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
    "$<BUILD_INTERFACE:${GTEST_CRITERIA_SQL_OUTPUT_ROOT}/include>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
)
if(NOT GTEST_CRITERIA_WITH_POSTGRESQL)
  target_include_directories(gtest_criteria BEFORE PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src/sqlite_only")
endif()
target_compile_definitions(gtest_criteria
  PRIVATE
    GTEST_CRITERIA_BUILD_SQL_DIR="${CMAKE_CURRENT_SOURCE_DIR}/sql/sqlite"
    GTEST_CRITERIA_INSTALL_SQL_DIR="${CMAKE_INSTALL_FULL_DATADIR}/gtest-criteria/sql/criteria"
    SQL_TEST_EVENT_LISTENER_BUILD_SQL_DIR="${GTEST_CRITERIA_SQL_OUTPUT_ROOT}/sql"
    SQL_TEST_EVENT_LISTENER_INSTALL_SQL_DIR="${CMAKE_INSTALL_FULL_DATADIR}/gtest-criteria/sql/listener"
  PUBLIC
    GTEST_CRITERIA_HAS_POSTGRESQL=$<BOOL:${GTEST_CRITERIA_WITH_POSTGRESQL}>
)
target_link_libraries(gtest_criteria
  PUBLIC
    "${GTEST_CRITERIA_GMOCK_TARGET}"
    SQLite3::SQLite3
)
if(GTEST_CRITERIA_WITH_POSTGRESQL)
  target_link_libraries(gtest_criteria PUBLIC PostgreSQL::PostgreSQL)
endif()

if(GTEST_CRITERIA_BUILD_EXAMPLE)
  add_subdirectory(example)
endif()

if(BUILD_TESTING)
  add_subdirectory(tests)
endif()

install(TARGETS gtest_criteria
  EXPORT GTestCriteriaTargets
  ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
  LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
  RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
  INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)
install(FILES
  include/criteria_policy.hpp
  include/test_fixture.hpp
  include/test_fixture.tpp
  "${GTEST_CRITERIA_SQL_OUTPUT_ROOT}/include/parse_argument.hpp"
  "${GTEST_CRITERIA_SQL_OUTPUT_ROOT}/include/register_listener.hpp"
  "${GTEST_CRITERIA_SQL_OUTPUT_ROOT}/include/sql_backend.hpp"
  "${GTEST_CRITERIA_SQL_OUTPUT_ROOT}/include/sql_statement_reader.hpp"
  "${GTEST_CRITERIA_SQL_OUTPUT_ROOT}/include/sql_test_event_listner.hpp"
  "${GTEST_CRITERIA_SQL_OUTPUT_ROOT}/include/sqlite_backend.hpp"
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/gtest-criteria"
)
if(GTEST_CRITERIA_WITH_POSTGRESQL)
  install(FILES
    "${GTEST_CRITERIA_SQL_OUTPUT_ROOT}/include/postgresql_backend.hpp"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/gtest-criteria"
  )
endif()
install(DIRECTORY "${GTEST_CRITERIA_SQL_OUTPUT_ROOT}/sql/"
  DESTINATION "${CMAKE_INSTALL_DATADIR}/gtest-criteria/sql/listener")
install(FILES sql/sqlite/criteria.sql
  DESTINATION "${CMAKE_INSTALL_DATADIR}/gtest-criteria/sql/criteria")

write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/GTestCriteriaConfigVersion.cmake"
  VERSION "${PROJECT_VERSION}"
  COMPATIBILITY SameMajorVersion
)
configure_package_config_file(
  cmake/GTestCriteriaConfig.cmake.in
  "${CMAKE_CURRENT_BINARY_DIR}/GTestCriteriaConfig.cmake"
  INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/GTestCriteria"
)
install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/GTestCriteriaConfig.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/GTestCriteriaConfigVersion.cmake"
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/GTestCriteria"
)
install(EXPORT GTestCriteriaTargets
  FILE GTestCriteriaTargets.cmake
  NAMESPACE GTestCriteria::
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/GTestCriteria"
)

set(CPACK_PACKAGE_NAME "gtest-criteria")
set(CPACK_PACKAGE_VENDOR "Andrew J. Sender")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_CONTACT "Andrew J. Sender")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_SOURCE_IGNORE_FILES "/build[^/]*/;/.git/;/.vscode/")
include(CPack)

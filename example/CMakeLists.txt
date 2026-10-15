cmake_minimum_required(VERSION 3.16)

project(gtest_criteria_example LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_XCODE_GENERATE_SCHEME ON)

include(ExternalProject)

set(PROJECT_ROOT "${CMAKE_CURRENT_LIST_DIR}/..")
set(SQL_OUTPUT_ROOT "${PROJECT_ROOT}/submodules/gtest_output-sql")
set(GTEST_SOURCE_DIR "${SQL_OUTPUT_ROOT}/submodules/googletest")
set(SQLITE_SOURCE_DIR "${SQL_OUTPUT_ROOT}/submodules/sqlite")
set(SQLITE_INSTALL_DIR "${CMAKE_CURRENT_BINARY_DIR}/sqlite")

if(NOT EXISTS "${GTEST_SOURCE_DIR}/CMakeLists.txt")
  message(FATAL_ERROR
    "GoogleTest is missing. Run: git submodule update --init --recursive")
endif()

if(NOT EXISTS "${SQLITE_SOURCE_DIR}/configure")
  message(FATAL_ERROR
    "SQLite is missing. Run: git submodule update --init --recursive")
endif()

find_program(SQLITE_MAKE_PROGRAM NAMES make gmake REQUIRED)
find_package(PostgreSQL REQUIRED)

add_subdirectory("${GTEST_SOURCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/googletest")

file(MAKE_DIRECTORY "${SQLITE_INSTALL_DIR}/include")
ExternalProject_Add(sqlite_build
  SOURCE_DIR "${SQLITE_SOURCE_DIR}"
  CONFIGURE_COMMAND
    <SOURCE_DIR>/configure
    --disable-shared
    --enable-static
    --prefix=<INSTALL_DIR>
  BUILD_COMMAND "${SQLITE_MAKE_PROGRAM}"
  INSTALL_COMMAND "${SQLITE_MAKE_PROGRAM}" install
  BUILD_BYPRODUCTS "${SQLITE_INSTALL_DIR}/lib/libsqlite3.a"
  INSTALL_DIR "${SQLITE_INSTALL_DIR}"
)

add_library(sqlite STATIC IMPORTED GLOBAL)
set_target_properties(sqlite PROPERTIES
  IMPORTED_LOCATION "${SQLITE_INSTALL_DIR}/lib/libsqlite3.a"
  INTERFACE_INCLUDE_DIRECTORIES "${SQLITE_INSTALL_DIR}/include"
)
add_dependencies(sqlite sqlite_build)

file(GLOB_RECURSE EXAMPLE_SOURCES CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_LIST_DIR}/*.cpp"
  "${PROJECT_ROOT}/src/*.cpp"
  "${SQL_OUTPUT_ROOT}/src/*.cpp"
)

add_executable(example ${EXAMPLE_SOURCES})
set_source_files_properties(
  "${PROJECT_ROOT}/include/test_fixture.tpp"
  PROPERTIES HEADER_FILE_ONLY TRUE
)
target_sources(example PRIVATE
  "${PROJECT_ROOT}/include/test_fixture.tpp"
)
target_include_directories(example PRIVATE
  "${PROJECT_ROOT}/include"
  "${SQL_OUTPUT_ROOT}/include"
)
target_compile_definitions(example PRIVATE
  SQL_TEST_EVENT_LISTENER_SQL_DIR="${SQL_OUTPUT_ROOT}/sql"
  GTEST_CRITERIA_SQL_DIR="${PROJECT_ROOT}/sql/sqlite"
)
target_link_libraries(example PRIVATE
  sqlite
  PostgreSQL::PostgreSQL
  gmock
)

# XCode specific settings
set(EXAMPLE_SCHEME_ARGUMENTS
    "--gtest_repeat=5"
    "--gtest_output=sqlite:test_output.db"
    "--criteria_disable_expect_performant"
    "--criteria_disable_expect_integrity"
    "--criteria_disable_expect_stable"
    "--no-exert_failure"
)

set_target_properties(example PROPERTIES
    XCODE_SCHEME_ARGUMENTS "${EXAMPLE_SCHEME_ARGUMENTS}"
)

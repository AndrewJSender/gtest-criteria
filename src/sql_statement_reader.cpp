// Copyright (c) 2026 Andrew J. Sender.
// All rights reserved.

#include "sql_statement_reader.hpp"

#include "embedded_sql.hpp"

#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <utility>

namespace testing {
namespace {

std::string EmbeddedSql(const std::filesystem::path& path) {
  const std::string name = path.filename().string();
  if (name == "environment.sql") {
    return gtest_criteria::embedded_sql::environment;
  }
  if (name == "iteration.sql") {
    return gtest_criteria::embedded_sql::iteration;
  }
  if (name == "program.sql") {
    return gtest_criteria::embedded_sql::program;
  }
  if (name == "suite.sql") {
    return gtest_criteria::embedded_sql::suite;
  }
  if (name == "test.sql") {
    return gtest_criteria::embedded_sql::test;
  }
  if (name == "test_result_part.sql") {
    return gtest_criteria::embedded_sql::test_result_part;
  }
  if (name == "criteria.sql") {
    return gtest_criteria::embedded_sql::criteria;
  }
  return {};
}

void ReplaceAll(std::string& text, const std::string& from,
                const std::string& to) {
  std::size_t position = 0;
  while ((position = text.find(from, position)) != std::string::npos) {
    text.replace(position, from.size(), to);
    position += to.size();
  }
}

std::string ApplyDialect(std::string sql, const char* dialect) {
  const bool is_postgresql = std::string(dialect) == "postgresql";
  ReplaceAll(sql, "{{PK}}",
             is_postgresql
                 ? "BIGINT GENERATED ALWAYS AS IDENTITY PRIMARY KEY"
                 : "INTEGER PRIMARY KEY AUTOINCREMENT");
  ReplaceAll(sql, "{{INT}}", is_postgresql ? "BIGINT" : "INTEGER");
  ReplaceAll(sql, "{{RETURNING}}", is_postgresql ? "\nRETURNING id" : "");
  if (is_postgresql) {
    std::string rewritten;
    rewritten.reserve(sql.size());
    int parameter = 1;
    for (char c : sql) {
      if (c == '?') {
        rewritten += '$' + std::to_string(parameter++);
      } else {
        rewritten += c;
      }
    }
    sql = std::move(rewritten);
  }
  return sql;
}

std::string ReadRawSql(const char* file_name) {
  const std::filesystem::path requested_path(file_name);
  const std::filesystem::path candidates[] = {
      requested_path,
      std::filesystem::path(SQL_TEST_EVENT_LISTENER_BUILD_SQL_DIR) /
          requested_path,
      std::filesystem::path(GTEST_CRITERIA_BUILD_SQL_DIR) / requested_path,
      std::filesystem::path(SQL_TEST_EVENT_LISTENER_INSTALL_SQL_DIR) /
          requested_path,
      std::filesystem::path(GTEST_CRITERIA_INSTALL_SQL_DIR) / requested_path,
  };
  for (const auto& sql_path : candidates) {
    std::ifstream file(sql_path);
    if (file) {
      return std::string{std::istreambuf_iterator<char>(file),
                         std::istreambuf_iterator<char>()};
    }
  }
  std::string embedded = EmbeddedSql(requested_path);
  if (!embedded.empty()) {
    return embedded;
  }
  throw std::runtime_error("Unable to locate SQL file '" +
                           requested_path.string() + "'");
}

}  // namespace

std::string ReadSqlSchema(const char* dialect, const char* file_name) {
  std::string sql = ReadRawSql(file_name);
  const std::size_t schema_end = sql.find("\n-- ", sql.find(';'));
  if (schema_end != std::string::npos) {
    sql.resize(schema_end);
  }
  return ApplyDialect(std::move(sql), dialect);
}

std::string ReadSqlStatement(const char* dialect, const char* file_name,
                             const char* marker) {
  const std::string sql = ReadRawSql(file_name);
  const std::string marker_text = std::string("-- ") + marker + "\n";
  const std::size_t marker_start = sql.find(marker_text);
  if (marker_start == std::string::npos) {
    throw std::runtime_error("SQL statement '" + std::string(marker) +
                             "' not found in '" + file_name + "'");
  }
  const std::size_t statement_start = marker_start + marker_text.size();
  const std::size_t statement_end = sql.find(';', statement_start);
  if (statement_end == std::string::npos) {
    throw std::runtime_error("SQL statement '" + std::string(marker) +
                             "' in '" + file_name + "' has no terminator");
  }
  return ApplyDialect(
      sql.substr(statement_start, statement_end - statement_start + 1),
      dialect);
}

}  // namespace testing

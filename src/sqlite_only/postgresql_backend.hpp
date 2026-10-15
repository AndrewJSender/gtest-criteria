// Copyright (c) 2026 Andrew J. Sender.
// All rights reserved.

#ifndef GTEST_CRITERIA_POSTGRESQL_BACKEND_STUB_HPP
#define GTEST_CRITERIA_POSTGRESQL_BACKEND_STUB_HPP

#include <cstdint>
#include <string>
#include <vector>

#include "sql_backend.hpp"

namespace testing {

class PostgreSqlBackend : public SqlBackend {
 public:
  explicit PostgreSqlBackend(const std::string& url);
  ~PostgreSqlBackend() override;

  void Execute(const char*, const char*, const std::vector<std::string>&,
               const std::vector<std::int64_t>&) override;
  std::vector<std::vector<std::string>> Query(
      const char*, const char*, const std::vector<std::string>&,
      const std::vector<std::int64_t>&) override;
  std::int64_t InsertRow(const char*, const char*,
                         const std::vector<std::string>&,
                         const std::vector<std::int64_t>&) override;
};

}  // namespace testing

#endif  // GTEST_CRITERIA_POSTGRESQL_BACKEND_STUB_HPP

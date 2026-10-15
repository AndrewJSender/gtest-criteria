// Copyright (c) 2026 Andrew J. Sender.
// All rights reserved.

#include "postgresql_backend.hpp"

#include <stdexcept>

namespace testing {
namespace {

[[noreturn]] void ThrowPostgreSqlDisabled() {
  throw std::runtime_error(
      "PostgreSQL support is disabled; configure with "
      "-DGTEST_CRITERIA_WITH_POSTGRESQL=ON");
}

}  // namespace

PostgreSqlBackend::PostgreSqlBackend(const std::string&) {
  ThrowPostgreSqlDisabled();
}

PostgreSqlBackend::~PostgreSqlBackend() = default;

void PostgreSqlBackend::Execute(const char*, const char*,
                                const std::vector<std::string>&,
                                const std::vector<std::int64_t>&) {
  ThrowPostgreSqlDisabled();
}

std::vector<std::vector<std::string>> PostgreSqlBackend::Query(
    const char*, const char*, const std::vector<std::string>&,
    const std::vector<std::int64_t>&) {
  ThrowPostgreSqlDisabled();
}

std::int64_t PostgreSqlBackend::InsertRow(
    const char*, const char*, const std::vector<std::string>&,
    const std::vector<std::int64_t>&) {
  ThrowPostgreSqlDisabled();
}

}  // namespace testing

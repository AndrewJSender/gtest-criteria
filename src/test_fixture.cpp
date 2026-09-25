

#include <filesystem>
#include "sqlite3.h"
#include "test_fixture.hpp"

void GTestCriteriaFixture::ConfigureSqlDb(std::optional<SqlOutput> arg) {
    if (arg->type == SqlOutputType::SQLite) {
        const std::filesystem::path db_path = arg->connection;
        if (!std::filesystem::exists(db_path)) {
            throw std::runtime_error("Database path does not exist: " + db_path.string());
        }

        const int result = sqlite3_open_v2(db_path.string().c_str(), &m_db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr);
        if (result != SQLITE_OK) {
            const std::string error = m_db != nullptr ? sqlite3_errmsg(m_db) : "unable to allocate SQLite handle";
            if (m_db != nullptr) {
                sqlite3_close(m_db);
                m_db = nullptr;
            }
            throw std::runtime_error("Unable to open SQLite database '" + db_path.string() + "': " + error);
        }
    }
}

void GTestCriteriaFixture::SetUp()
{
    test_timer = std::chrono::high_resolution_clock::now();
}

void GTestCriteriaFixture::TearDown()
{
    auto test_duration = std::chrono::high_resolution_clock::now() - test_timer;

    // TODO Retrieve average test duration of previous runs from
    // some external source and calculate the average test duration.
}

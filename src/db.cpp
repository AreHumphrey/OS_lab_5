#include "db.h"

#include <sqlite3.h>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <chrono>
#include <ctime>

namespace lab5 {

Database::Database() {}

Database::~Database() {
    if (db_) {
        sqlite3_close(db_);
    }
}

bool Database::open(const std::string& path, std::string& err) {
    int rc = sqlite3_open(path.c_str(), &db_);
    if (rc != SQLITE_OK) {
        err = "Can't open database: " + std::string(sqlite3_errmsg(db_));
        return false;
    }
    
    // Создаем таблицы, если их нет
    const char* create_sql = 
        "CREATE TABLE IF NOT EXISTS measurements ("
        "    epoch_ms INTEGER PRIMARY KEY,"
        "    value REAL NOT NULL,"
        "    timestamp TEXT NOT NULL"
        ");"
        "CREATE TABLE IF NOT EXISTS hourly_avg ("
        "    epoch_ms INTEGER PRIMARY KEY,"
        "    value REAL NOT NULL,"
        "    timestamp TEXT NOT NULL"
        ");"
        "CREATE TABLE IF NOT EXISTS daily_avg ("
        "    epoch_ms INTEGER PRIMARY KEY,"
        "    value REAL NOT NULL,"
        "    timestamp TEXT NOT NULL"
        ");";
    
    char* zErrMsg = 0;
    rc = sqlite3_exec(db_, create_sql, 0, 0, &zErrMsg);
    if (rc != SQLITE_OK) {
        err = "SQL error: " + std::string(zErrMsg);
        sqlite3_free(zErrMsg);
        return false;
    }
    
    return true;
}

bool Database::insert_measurement(const Sample& s, std::string& err) {
    const char* sql = "INSERT OR REPLACE INTO measurements (epoch_ms, value, timestamp) VALUES (?, ?, ?)";
    sqlite3_stmt* stmt;
    
    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        err = "Prepare failed: " + std::string(sqlite3_errmsg(db_));
        return false;
    }
    
    sqlite3_bind_int64(stmt, 1, std::chrono::duration_cast<std::chrono::milliseconds>(s.ts.time_since_epoch()).count());
    sqlite3_bind_double(stmt, 2, s.value);
    sqlite3_bind_text(stmt, 3, iso_time(s.ts).c_str(), -1, SQLITE_STATIC);
    
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        err = "Insert failed: " + std::string(sqlite3_errmsg(db_));
        sqlite3_finalize(stmt);
        return false;
    }
    
    sqlite3_finalize(stmt);
    return true;
}

bool Database::insert_hourly(const Sample& s, std::string& err) {
    const char* sql = "INSERT OR REPLACE INTO hourly_avg (epoch_ms, value, timestamp) VALUES (?, ?, ?)";
    sqlite3_stmt* stmt;
    
    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        err = "Prepare failed: " + std::string(sqlite3_errmsg(db_));
        return false;
    }
    
    sqlite3_bind_int64(stmt, 1, std::chrono::duration_cast<std::chrono::milliseconds>(s.ts.time_since_epoch()).count());
    sqlite3_bind_double(stmt, 2, s.value);
    sqlite3_bind_text(stmt, 3, iso_time(s.ts).c_str(), -1, SQLITE_STATIC);
    
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        err = "Insert failed: " + std::string(sqlite3_errmsg(db_));
        sqlite3_finalize(stmt);
        return false;
    }
    
    sqlite3_finalize(stmt);
    return true;
}

bool Database::insert_daily(const Sample& s, std::string& err) {
    const char* sql = "INSERT OR REPLACE INTO daily_avg (epoch_ms, value, timestamp) VALUES (?, ?, ?)";
    sqlite3_stmt* stmt;
    
    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        err = "Prepare failed: " + std::string(sqlite3_errmsg(db_));
        return false;
    }
    
    sqlite3_bind_int64(stmt, 1, std::chrono::duration_cast<std::chrono::milliseconds>(s.ts.time_since_epoch()).count());
    sqlite3_bind_double(stmt, 2, s.value);
    sqlite3_bind_text(stmt, 3, iso_time(s.ts).c_str(), -1, SQLITE_STATIC);
    
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        err = "Insert failed: " + std::string(sqlite3_errmsg(db_));
        sqlite3_finalize(stmt);
        return false;
    }
    
    sqlite3_finalize(stmt);
    return true;
}

std::optional<Sample> Database::latest_measurement(std::string& err) {
    const char* sql = "SELECT epoch_ms, value, timestamp FROM measurements ORDER BY epoch_ms DESC LIMIT 1";
    sqlite3_stmt* stmt;
    
    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        err = "Prepare failed: " + std::string(sqlite3_errmsg(db_));
        return std::nullopt;
    }
    
    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        Sample s;
        s.ts = TimePoint(std::chrono::milliseconds(sqlite3_column_int64(stmt, 0)));
        s.value = sqlite3_column_double(stmt, 1);
        sqlite3_finalize(stmt);
        return s;
    }
    
    sqlite3_finalize(stmt);
    return std::nullopt;
}

bool Database::query_range(const std::string& table, std::int64_t start_ms, std::int64_t end_ms, std::vector<Sample>& out, std::string& err) {
    std::string sql = "SELECT epoch_ms, value, timestamp FROM " + table + 
                      " WHERE epoch_ms >= ? AND epoch_ms <= ? ORDER BY epoch_ms";
    
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        err = "Prepare failed: " + std::string(sqlite3_errmsg(db_));
        return false;
    }
    
    sqlite3_bind_int64(stmt, 1, start_ms);
    sqlite3_bind_int64(stmt, 2, end_ms);
    
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        Sample s;
        s.ts = TimePoint(std::chrono::milliseconds(sqlite3_column_int64(stmt, 0)));
        s.value = sqlite3_column_double(stmt, 1);
        out.push_back(s);
    }
    
    if (rc != SQLITE_DONE) {
        err = "Query failed: " + std::string(sqlite3_errmsg(db_));
        sqlite3_finalize(stmt);
        return false;
    }
    
    sqlite3_finalize(stmt);
    return true;
}

bool Database::prune_measurements(std::int64_t cutoff_ms, std::string& err) {
    std::string sql = "DELETE FROM measurements WHERE epoch_ms < ?";
    sqlite3_stmt* stmt;
    
    int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        err = "Prepare failed: " + std::string(sqlite3_errmsg(db_));
        return false;
    }
    
    sqlite3_bind_int64(stmt, 1, cutoff_ms);
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    return (rc == SQLITE_DONE);
}

bool Database::prune_hourly(std::int64_t cutoff_ms, std::string& err) {
    std::string sql = "DELETE FROM hourly_avg WHERE epoch_ms < ?";
    sqlite3_stmt* stmt;
    
    int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        err = "Prepare failed: " + std::string(sqlite3_errmsg(db_));
        return false;
    }
    
    sqlite3_bind_int64(stmt, 1, cutoff_ms);
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    return (rc == SQLITE_DONE);
}

bool Database::prune_daily_current_year(std::string& err) {
    // Получаем текущий год
    auto now = Clock::now();
    std::tm tm{};
    auto tt = Clock::to_time_t(now);
    
#ifdef _WIN32
    localtime_s(&tm, &tt);
#else
    localtime_r(&tt, &tm);
#endif
    
    int current_year = tm.tm_year + 1900;
    std::string sql = "DELETE FROM daily_avg WHERE strftime('%Y', timestamp) < '" + std::to_string(current_year) + "'";
    
    char* zErrMsg = 0;
    int rc = sqlite3_exec(db_, sql.c_str(), 0, 0, &zErrMsg);
    if (rc != SQLITE_OK) {
        err = "SQL error: " + std::string(zErrMsg);
        sqlite3_free(zErrMsg);
        return false;
    }
    
    return true;
}

}  // namespace lab5

#pragma once

#include <functional>
#include <optional>
#include <string>
#include <vector>

#include "common.h"
#include "sample.h"

struct sqlite3;

namespace lab5 {

class Database {
public:
    Database();
    ~Database();
    bool open(const std::string& path, std::string& err);

    bool insert_measurement(const Sample& s, std::string& err);
    bool insert_hourly(const Sample& s, std::string& err);
    bool insert_daily(const Sample& s, std::string& err);

    std::optional<Sample> latest_measurement(std::string& err);
    bool query_range(const std::string& table, std::int64_t start_ms, std::int64_t end_ms, std::vector<Sample>& out, std::string& err);

    bool prune_measurements(std::int64_t cutoff_ms, std::string& err);
    bool prune_hourly(std::int64_t cutoff_ms, std::string& err);
    bool prune_daily_current_year(std::string& err);

private:
    bool exec(const std::string& sql, std::string& err);

    sqlite3* db_ = nullptr;
};

}  // namespace lab5

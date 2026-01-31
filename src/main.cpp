#include <atomic>
#include <chrono>
#include <csignal>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <thread>
#include <vector>
#include <fstream>
#include <filesystem>

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#else
#include <unistd.h>
#endif

#include "common.h"
#include "db.h"
#include "http_server.h"
#include "sample.h"
#include "simulator.h"

namespace fs = std::filesystem;
using namespace std::chrono;

namespace lab5 {
namespace {

std::atomic<bool> g_running{true};

void signal_handler(int) { 
    g_running = false; 
    std::cout << "\n[Получен сигнал завершения, остановка сервера...]\n";
}

class TemperatureSimulator {
public:
    TemperatureSimulator() : rng(static_cast<unsigned>(now_ms())) {}
    
    Sample generate() {
        static std::uniform_real_distribution<double> dist(-10.0, 35.0);
        return {Clock::now(), dist(rng)};
    }
    
private:
    std::mt19937 rng;
};

}  // namespace

int run_main(bool simulate) {
    std::signal(SIGINT, signal_handler);
#ifndef _WIN32
    std::signal(SIGTERM, signal_handler);
#endif

    std::cout << "lab5: Temperature logger started\n";
    std::cout << "Mode: " << (simulate ? "SIMULATION" : "STDIN") << "\n";
    std::cout << "Database: " << db_path() << "\n";
    std::cout << "Web interface: http://localhost:8080\n";
    std::cout << "Press Ctrl+C to stop\n\n";

    TemperatureSimulator simulator;
    Accum hour_acc;
    Accum day_acc;

    auto now = Clock::now();
    int last_hour = hour_of(now);
    int last_day = day_of_year(now);
    int last_year = year_of(now);

    int sample_count = 0;

    std::string db_err;
    Database db;
    if (!db.open(db_path(), db_err)) {
        std::cerr << "Failed to open database: " << db_err << std::endl;
        return 1;
    }

    auto flush_hour = [&](const TimePoint& ts) {
        if (hour_acc.count == 0) return;
        
        Sample avg{ts, hour_acc.avg()};
        if (!db.insert_hourly(avg, db_err)) {
            std::cerr << "Failed to insert hourly: " << db_err << std::endl;
        }
        
        auto cutoff = now_ms() - 30LL * 24 * 60 * 60 * 1000;
        if (!db.prune_hourly(cutoff, db_err)) {
            std::cerr << "Failed to prune hourly: " << db_err << std::endl;
        }
        
        hour_acc.reset();
    };

    auto flush_day = [&](const TimePoint& ts) {
        if (day_acc.count == 0) return;
        
        Sample avg{ts, day_acc.avg()};
        if (!db.insert_daily(avg, db_err)) {
            std::cerr << "Failed to insert daily: " << db_err << std::endl;
        }
        
        if (!db.prune_daily_current_year(db_err)) {
            std::cerr << "Failed to prune daily: " << db_err << std::endl;
        }
        
        day_acc.reset();
    };

    auto process_sample = [&](const Sample& s) {
        const int h = hour_of(s.ts);
        const int d = day_of_year(s.ts);
        const int y = year_of(s.ts);

        if (h != last_hour) {
            flush_hour(s.ts);
            last_hour = h;
        }

        if (d != last_day) {
            flush_day(s.ts);
            last_day = d;
            last_year = y;
        }

        hour_acc.add(s.value);
        day_acc.add(s.value);

        if (!db.insert_measurement(s, db_err)) {
            std::cerr << "Failed to insert measurement: " << db_err << std::endl;
        }
        
        ++sample_count;
        if (sample_count % 50 == 0) {
            auto cutoff = now_ms() - 24LL * 60 * 60 * 1000;
            if (!db.prune_measurements(cutoff, db_err)) {
                std::cerr << "Failed to prune measurements: " << db_err << std::endl;
            }
        }
        
        std::cout << "[" << iso_time(s.ts) << "] "
                  << "Temp: " << s.value << "°C | "
                  << "Hour avg: " << hour_acc.avg() << "°C | "
                  << "Day avg: " << day_acc.avg() << "°C\n";
    };

    auto http_handler = [&](const std::string& path) -> std::pair<std::string, std::string> {
        if (path.find("/api/current") == 0) {
            std::string db_err;
            auto latest = db.latest_measurement(db_err);
            if (!latest) {
                return {"{\"error\":\"No data available\"}", "application/json"};
            }
            
            std::ostringstream oss;
            oss << "{\"epoch_ms\":" << std::chrono::duration_cast<std::chrono::milliseconds>(latest->ts.time_since_epoch()).count()
                << ",\"value\":" << latest->value
                << ",\"time\":\"" << iso_time(latest->ts) << "\"}";
            
            return {oss.str(), "application/json"};
        }
        
        if (path.find("/api/stats") == 0) {
            std::string params = path.substr(path.find('?') + 1);
            std::string bucket;
            std::int64_t start_ms = 0;
            std::int64_t end_ms = now_ms();
            
            size_t pos = 0;
            while ((pos = params.find('&')) != std::string::npos) {
                std::string param = params.substr(0, pos);
                params = params.substr(pos + 1);
                
                if (param.find("bucket=") == 0) {
                    bucket = param.substr(7);
                } else if (param.find("start=") == 0) {
                    start_ms = std::stoll(param.substr(6));
                } else if (param.find("end=") == 0) {
                    end_ms = std::stoll(param.substr(4));
                }
            }
            
            if (!params.empty()) {
                if (params.find("bucket=") == 0) {
                    bucket = params.substr(7);
                } else if (params.find("start=") == 0) {
                    start_ms = std::stoll(params.substr(6));
                } else if (params.find("end=") == 0) {
                    end_ms = std::stoll(params.substr(4));
                }
            }
            
            if (bucket.empty() || start_ms == 0) {
                return {"{\"error\":\"Invalid parameters\"}", "application/json"};
            }
            
            std::string table;
            if (bucket == "raw") {
                table = "measurements";
            } else if (bucket == "hourly") {
                table = "hourly_avg";
            } else if (bucket == "daily") {
                table = "daily_avg";
            } else {
                return {"{\"error\":\"Invalid bucket\"}", "application/json"};
            }
            
            std::vector<Sample> samples;
            std::string db_err;
            if (!db.query_range(table, start_ms, end_ms, samples, db_err)) {
                return {"{\"error\":\"Database error: " + db_err + "\"}", "application/json"};
            }
            
            std::ostringstream oss;
            oss << "{\"data\":[";
            for (size_t i = 0; i < samples.size(); ++i) {
                if (i > 0) oss << ",";
                oss << "[" 
                    << std::chrono::duration_cast<std::chrono::milliseconds>(samples[i].ts.time_since_epoch()).count()
                    << "," << samples[i].value << "]";
            }
            oss << "]}";
            
            return {oss.str(), "application/json"};
        }
        
      
        std::string base_path = web_root();
        std::string filepath = base_path + "/public";
        
        if (path == "/" || path.empty()) {
            filepath += "/index.html";
        } else {
            if (path[0] == '/') {
                filepath += path;
            } else {
                filepath += "/" + path;
            }
        }
        
        std::ifstream file(filepath, std::ios::binary);
        if (file.is_open()) {
            std::ostringstream oss;
            oss << file.rdbuf();
            std::string content = oss.str();
            file.close();
            
            std::string content_type = "text/html";
            if (filepath.find(".css") != std::string::npos) content_type = "text/css";
            else if (filepath.find(".js") != std::string::npos) content_type = "application/javascript";
            else if (filepath.find(".json") != std::string::npos) content_type = "application/json";
            
            return {content, content_type};
        }
        
      
        std::cerr << "[DEBUG] File not found: " << filepath << std::endl;
        std::cerr << "[DEBUG] Current dir: " << fs::current_path().string() << std::endl;
        std::cerr << "[DEBUG] Web root: " << base_path << std::endl;
        std::cerr << "[DEBUG] Looking for: " << (base_path + "/public/index.html") << std::endl;
        
        return {"<h1>404 Not Found</h1><p>File: " + filepath + "</p><p>Web root: " + base_path + "</p>", "text/html"};
    };

    HttpServer http_server;
    std::string err;
    if (!http_server.start(8080, http_handler, err)) {
        std::cerr << "Failed to start HTTP server: " << err << std::endl;
        return 1;
    }

    while (g_running) {
        Sample s;
        
        if (simulate) {
            auto start = Clock::now();
            while (g_running && (Clock::now() - start) < seconds(2)) {
                std::this_thread::sleep_for(milliseconds(100));
            }
            if (!g_running) break;
            
            s = simulator.generate();
        } else {
            std::string line;
            if (!std::getline(std::cin, line)) {
                std::this_thread::sleep_for(milliseconds(100));
                continue;
            }
            
            std::istringstream iss(line);
            double val;
            if (!(iss >> val)) {
                std::cerr << "Invalid input, expected number. Got: " << line << "\n";
                continue;
            }
            s.ts = Clock::now();
            s.value = val;
        }

        process_sample(s);
    }

    flush_hour(Clock::now());
    flush_day(Clock::now());
    http_server.stop();
    
    std::cout << "\nlab5: Temperature logger stopped\n";
    return 0;
}

}  // namespace lab5

int main(int argc, char* argv[]) {
    bool simulate = false;
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--simulate" || arg == "-s") {
            simulate = true;
        }
    }
    
    return lab5::run_main(simulate);
}

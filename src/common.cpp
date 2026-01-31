#include "common.h"

#include <filesystem>
#include <iomanip>
#include <sstream>

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace fs = std::filesystem;

namespace lab5 {

std::string iso_time(const TimePoint& tp) {
    using namespace std::chrono;
    auto tt = Clock::to_time_t(tp);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &tt);
#else
    localtime_r(&tt, &tm);
#endif
    auto ms = duration_cast<milliseconds>(tp.time_since_epoch()) % 1000;
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") 
        << '.' << std::setw(3) << std::setfill('0') << ms.count();
    return oss.str();
}

std::string data_dir() {
   
    return fs::current_path().string();
}

std::string db_path() {
    return (fs::path(data_dir()) / "lab5.db").string();
}

std::string web_root() {
   
    fs::path current = fs::current_path();
    fs::path web_dir;
    
  
    if (fs::exists(current / "web" / "public")) {
        web_dir = current / "web";
    } else if (fs::exists(current.parent_path() / "web" / "public")) {
        web_dir = current.parent_path() / "web";
    } else if (fs::exists(current.parent_path().parent_path() / "web" / "public")) {
        web_dir = current.parent_path().parent_path() / "web";
    } else {

        web_dir = current / "web";
    }
    
    return web_dir.string();
}

}  // namespace lab5

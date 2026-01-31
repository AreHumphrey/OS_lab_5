#include "sample.h"

namespace lab5 {

int hour_of(const TimePoint& tp) {
    auto tt = Clock::to_time_t(tp);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &tt);
#else
    localtime_r(&tt, &tm);
#endif
    return tm.tm_hour;
}

int day_of_year(const TimePoint& tp) {
    auto tt = Clock::to_time_t(tp);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &tt);
#else
    localtime_r(&tt, &tm);
#endif
    return tm.tm_yday;
}

int year_of(const TimePoint& tp) {
    auto tt = Clock::to_time_t(tp);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &tt);
#else
    localtime_r(&tt, &tm);
#endif
    return tm.tm_year + 1900;
}

}  // namespace lab5

#pragma once

#include <cstddef>

#include "common.h"

namespace lab5 {

struct Sample {
    TimePoint ts;
    double value = 0.0;
};

struct Accum {
    double sum = 0.0;
    std::size_t count = 0;
    void add(double v) { sum += v; ++count; }
    double avg() const { return count ? sum / count : 0.0; }
    void reset() { sum = 0.0; count = 0; }
};

int hour_of(const TimePoint& tp);
int day_of_year(const TimePoint& tp);
int year_of(const TimePoint& tp);

}  // namespace lab5

#pragma once

#include <optional>
#include <string>

#include "sample.h"

namespace lab5 {

std::string format_line(const Sample& s);
std::optional<std::int64_t> parse_epoch_ms(const std::string& line);
void append_line(const std::string& path, const std::string& line);
void prune_log(const std::string& path, std::int64_t cutoff_ms);

}  // namespace lab5

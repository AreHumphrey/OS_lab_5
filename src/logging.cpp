#include "logging.h"

namespace lab5 {

std::string format_line(const Sample& s) {
    // Не используется — данные сохраняются в БД, а не в файлы
    return "";
}

std::optional<std::int64_t> parse_epoch_ms(const std::string& line) {
    // Не используется — данные читаются из БД
    return std::nullopt;
}

void append_line(const std::string& path, const std::string& line) {
    // Не используется — данные сохраняются в БД
}

void prune_log(const std::string& path, std::int64_t cutoff_ms) {
    // Не используется — очистка выполняется в БД
}

}  // namespace lab5

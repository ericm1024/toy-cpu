#include "log.h"

#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iterator>
#include <optional>
#include <string>
#include <string_view>

#define ENUM_DEF_FILE_NAME "log_level_def.h"
#include "enum_def.h"

static std::atomic<log_level> & get_log_level()
{
    static std::atomic<log_level> g_log_level{[]() -> log_level {
        std::optional<log_level> level;
        if (char const * env = getenv("CPU_LOG_LEVEL")) {
            level = from_str<log_level>(env);
        }
        return level.value_or(log_level::info);
    }()};
    return g_log_level;
}

logger::logger(char const * prefix)
    : prefix_(prefix)
{ }

void logger::vlog(log_level level, std::string_view fmt, std::format_args args)
{
    if (get_log_level().load(std::memory_order_relaxed) > level) {
        return;
    }

    std::string_view level_str = to_str(level);
    printf("%s %.*s: ", prefix_, static_cast<int>(level_str.size()), level_str.data());
    std::string str = vformat(fmt, args);
    printf("%s\n", str.c_str());
    if (level == log_level::abort) {
        fflush(stdout);
        std::abort();
    }
}

void set_log_level(log_level level)
{
    get_log_level() = level;
}

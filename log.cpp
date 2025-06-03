#include "log.h"

#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <optional>
#include <string_view>

static constinit char const * const level_to_str[]{
#define MAKE_ENTRY(level) [static_cast<uint8_t>(log_level::level)] = #level

    MAKE_ENTRY(debug),
    MAKE_ENTRY(info),
    MAKE_ENTRY(err),
    MAKE_ENTRY(abort),
};

static char const * to_str(log_level level)
{
    uint8_t index = static_cast<uint8_t>(level);
    if (index >= std::size(level_to_str)) {
        return "unknown";
    }
    return level_to_str[index];
}

static std::optional<log_level> from_str(char const * str)
{
    for (size_t i = 0; i < std::size(level_to_str); ++i) {
        if (strcmp(level_to_str[i], str) == 0) {
            return static_cast<log_level>(i);
        }
    }
    return std::nullopt;
}

static std::atomic<log_level> & get_log_level()
{
    static std::atomic<log_level> g_log_level{[]() -> log_level {
        std::optional<log_level> level;
        if (char const * env = getenv("CPU_LOG_LEVEL")) {
            level = from_str(env);
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

    printf("%s %s: ", prefix_, to_str(level));
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

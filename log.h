#pragma once

#include <cstdint>
#include <format>
#include <source_location>

enum class log_level : uint8_t
{
    debug,
    info,
    err,
    abort
};

struct logger
{
    logger(logger const &) = delete;
    logger & operator=(logger const &) = delete;

    logger(char const * prefix);

    // XXX: would like to use this constructor, but doesn't do what it should due to a clang bug
    // (https://github.com/llvm/llvm-project/issues/56379 maybe?)
#if 0
    logger(std::source_location loc = std::source_location::current())
        : logger{loc.file_name()}
    { }
#endif

    void vlog(log_level level, std::string_view fmt, std::format_args args);

    template <class... Args>
    void log(log_level level, std::format_string<Args...> fmt, Args &&... args)
    {
        vlog(level, fmt.get(), std::make_format_args(args...));
    }

    template <class... Args>
    void debug(std::format_string<Args...> fmt, Args &&... args)
    {
        vlog(log_level::debug, fmt.get(), std::make_format_args(args...));
    }

    template <class... Args>
    void info(std::format_string<Args...> fmt, Args &&... args)
    {
        vlog(log_level::info, fmt.get(), std::make_format_args(args...));
    }

    template <class... Args>
    void err(std::format_string<Args...> fmt, Args &&... args)
    {
        vlog(log_level::err, fmt.get(), std::make_format_args(args...));
    }

    template <class... Args>
    void abort(std::format_string<Args...> fmt, Args &&... args)
    {
        vlog(log_level::abort, fmt.get(), std::make_format_args(args...));
    }

private:
    char const * const prefix_;
};

void set_log_level(log_level level);

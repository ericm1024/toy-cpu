#pragma once

#include "cpu_base.h"

#include <cstddef>
#include <format>
#include <optional>
#include <stdint.h>
#include <string_view>

#define ENUM_DEF_FILE_NAME "reg_def.h"
#include "enum_decl.h" // IWYU pragma: export
using enum reg;
static size_t constexpr k_num_registers = std::to_underlying(r15) + 1;

static size_t constexpr k_reg_bits = 4;
static word_t constexpr k_reg_mask = (word_t{1} << k_reg_bits) - 1;

extern std::initializer_list<reg> const k_all_registers;

template <>
struct std::formatter<reg>
{
    template <class ParseContext>
    constexpr auto parse(ParseContext & ctx)
    {
        return ctx.begin();
    }

    template <class FormatContext>
    auto format(reg const & rr, FormatContext & ctx) const
    {
        return std::format_to(ctx.out(), "{}", to_str(rr));
    }
};

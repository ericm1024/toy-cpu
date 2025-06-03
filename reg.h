#pragma once

#include "cpu_base.h"

#include <cstddef>
#include <optional>
#include <string_view>

// 16 architectural registers
enum class reg : uint8_t
{
    r0,
    r1,
    r2,
    r3,
    r4,
    r5,
    r6,
    r7,
    r8,
    r9,
    r10,
    r11,
    r12,
    r13,
    r14,
    r15,
    num_registers,
};
using enum reg;

static size_t constexpr k_reg_bits = 4;
static word_t constexpr k_reg_mask = (word_t{1} << k_reg_bits) - 1;

static constexpr word_t raw(reg r)
{
    return static_cast<word_t>(r);
}

template <typename T>
std::optional<T> from_str(std::string_view str);

template <>
std::optional<reg> from_str<reg>(std::string_view str);

std::string_view to_str(reg rr);

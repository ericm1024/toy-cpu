#pragma once

#include "cpu_base.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>

// instruction opcodes
enum class opcode : uint8_t
{
    set,
    store,
    load,
    add,
    sub,
    halt,
    compare,
    jump,
    ijump,
    call,
};

static constexpr word_t raw(opcode op)
{
    return static_cast<word_t>(op);
}

char const * to_str(opcode op);

template <typename T>
std::optional<T> from_str(std::string_view str);
template <>
std::optional<opcode> from_str<opcode>(std::string_view str);

static size_t constexpr k_opcode_bits = sizeof(opcode) * 8;
static word_t constexpr k_opcode_mask = (word_t{1} << k_opcode_bits) - 1;

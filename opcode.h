#pragma once

#include "cpu_base.h"

#include <cstddef>
#include <cstdint>

// instruction opcodes
enum class opcode : uint8_t
{
    set,
    store,
    load,
    add,
    halt,
    compare,
    branch,
};

static constexpr word_t raw(opcode op)
{
    return static_cast<word_t>(op);
}

char const * to_str(opcode op);

static size_t constexpr k_opcode_bits = sizeof(opcode) * 8;
static word_t constexpr k_opcode_mask = (word_t{1} << k_opcode_bits) - 1;

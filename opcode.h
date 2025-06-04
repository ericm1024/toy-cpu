#pragma once

#include "cpu_base.h"

#include <cstddef>
#include <cstdint>

#define ENUM_DEF_FILE_NAME "opcode_def.h"
#include "enum_decl.h" // IWYU pragma: export

static size_t constexpr k_opcode_bits = sizeof(opcode) * 8;
static word_t constexpr k_opcode_mask = (word_t{1} << k_opcode_bits) - 1;

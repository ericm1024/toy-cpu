#pragma once

#include <bit>
#include <cstdint>
#include <type_traits>

using word_t = uint32_t;
using signed_word_t = std::make_signed_t<word_t>;

static word_t constexpr k_word_size = sizeof(word_t);
static word_t constexpr k_word_size_bits = std::bit_width(k_word_size - 1);

struct instr;

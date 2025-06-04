#pragma once

#include "cpu_base.h"
#include "opcode.h"
#include "reg.h"

#include <cassert>
#include <cstddef>
#include <format>

static size_t constexpr k_instr_bits = sizeof(word_t) * 8;

// Instructions are 32 bits
// First 8 bits are opcode, rest are opcode-dependent
struct instr
{
    explicit instr(word_t raw)
    {
        storage = raw;
    }

    instr(opcode op, word_t tail)
    {
        assert(tail < (1U << 24));
        storage = static_cast<uint8_t>(op) | (tail << k_opcode_bits);
    }

    opcode get_opcode() const
    {
        return static_cast<opcode>(storage & k_opcode_mask);
    }

    static word_t constexpr k_max_set_value
        = (1U << (k_instr_bits - k_opcode_bits - k_reg_bits)) - 1;

    // value can be at most 20 bits
    static instr set(reg dest, word_t value)
    {
        assert(value <= k_max_set_value);
        return {opcode::set, raw(dest) | (value << k_reg_bits)};
    }

    void decode_set(reg * dest, word_t * value) const
    {
        assert(get_opcode() == opcode::set);
        word_t tmp = storage >> k_opcode_bits;
        *dest = static_cast<reg>(tmp & k_reg_mask);
        *value = tmp >> k_reg_bits;
    }

private:
    enum class width_sel : uint8_t
    {
    };

    static word_t sel_to_width(width_sel width_sel)
    {
        uint8_t raw = static_cast<uint8_t>(width_sel);
        assert(raw < 3);
        return raw == 0 ? 1 : raw == 1 ? 2 : 4;
    }

    static width_sel width_to_sel(word_t width)
    {
        assert(width == 1 || width == 2 || width == 4);
        return static_cast<width_sel>(width == 1 ? 0 : width == 2 ? 1 : 2);
    }

    static instr load_store(opcode op, reg addr, reg src, word_t width)
    {
        assert(op == opcode::load || op == opcode::store);
        width_sel sel = width_to_sel(width);
        return {op,
                raw(addr) | raw(src) << (k_reg_bits)
                    | static_cast<word_t>(sel) << (2 * k_reg_bits)};
    }

    void decode_load_store(reg * addr, reg * src, word_t * width) const
    {
        word_t tmp = storage >> k_opcode_bits;
        *addr = static_cast<reg>(tmp & k_reg_mask);
        tmp >>= k_reg_bits;
        *src = static_cast<reg>(tmp & k_reg_mask);
        tmp >>= k_reg_bits;
        uint8_t raw_width_sel = tmp & 0x3;
        *width = sel_to_width(width_sel{raw_width_sel});
    }

public:
    static instr store(reg addr, reg src, word_t width)
    {
        return load_store(opcode::store, addr, src, width);
    }

    void decode_store(reg * addr, reg * src, word_t * width) const
    {
        assert(get_opcode() == opcode::store);
        decode_load_store(addr, src, width);
    }

    static instr store1(reg addr, reg src)
    {
        return store(addr, src, 1);
    }

    static instr store2(reg addr, reg src)
    {
        return store(addr, src, 2);
    }

    static instr store4(reg addr, reg src)
    {
        return store(addr, src, 4);
    }

    static instr load(reg addr, reg src, word_t width)
    {
        return load_store(opcode::load, addr, src, width);
    }

    void decode_load(reg * dest, reg * src, word_t * width) const
    {
        assert(get_opcode() == opcode::load);
        decode_load_store(dest, src, width);
    }

    static instr load1(reg dest, reg src)
    {
        return load(dest, src, 1);
    }

    static instr load2(reg dest, reg src)
    {
        return load(dest, src, 2);
    }

    static instr load4(reg dest, reg src)
    {
        return load(dest, src, 4);
    }

    static instr add(reg dest, reg op1, reg op2)
    {
        return {opcode::add, raw(dest) | (raw(op1) << k_reg_bits) | (raw(op2) << (2 * k_reg_bits))};
    }

    void decode_add(reg * dest, reg * op1, reg * op2) const
    {
        assert(get_opcode() == opcode::add);
        word_t tmp = storage >> k_opcode_bits;
        *dest = static_cast<reg>(tmp & k_reg_mask);
        tmp >>= k_reg_bits;
        *op1 = static_cast<reg>(tmp & k_reg_mask);
        tmp >>= k_reg_bits;
        *op2 = static_cast<reg>(tmp & k_reg_mask);
    }

    static instr sub(reg dest, reg op1, reg op2)
    {
        return {opcode::sub, raw(dest) | (raw(op1) << k_reg_bits) | (raw(op2) << (2 * k_reg_bits))};
    }

    void decode_sub(reg * dest, reg * op1, reg * op2) const
    {
        assert(get_opcode() == opcode::sub);
        word_t tmp = storage >> k_opcode_bits;
        *dest = static_cast<reg>(tmp & k_reg_mask);
        tmp >>= k_reg_bits;
        *op1 = static_cast<reg>(tmp & k_reg_mask);
        tmp >>= k_reg_bits;
        *op2 = static_cast<reg>(tmp & k_reg_mask);
    }

    static instr halt()
    {
        return {opcode::halt, 0};
    }

    static instr compare(reg op1, reg op2)
    {
        return {opcode::compare, raw(op1) | (raw(op2) << k_reg_bits)};
    }

    void decode_compare(reg * op1, reg * op2) const
    {
        assert(get_opcode() == opcode::compare);
        word_t tmp = storage >> k_opcode_bits;
        *op1 = static_cast<reg>(tmp & k_reg_mask);
        tmp >>= k_reg_bits;
        *op2 = static_cast<reg>(tmp & k_reg_mask);
    }

    enum class cmp_flag : uint8_t
    {
        eq,
        ne,
        gt,
        ge,
        lt,
        le,
        unc,
    };
    using enum cmp_flag;

    static std::initializer_list<cmp_flag> const k_all_cmp_flags;

    static size_t constexpr k_cmp_flag_bits = 4;

    // we encode into this may bits
    static size_t constexpr k_jump_offset_encode_bits
        = k_instr_bits - k_opcode_bits - k_cmp_flag_bits;

    // ... but users can specify this many bits, since instruction offsets must be divisible
    // by k_word_size
    static size_t constexpr k_jump_offset_bits = k_jump_offset_encode_bits + k_word_size_bits;

    static signed_word_t constexpr k_jump_max_offset
        = k_word_size * ((1 << (k_jump_offset_encode_bits - 1)) - 1);

    static signed_word_t constexpr k_jump_min_offset = -k_jump_max_offset;

    static instr jump(cmp_flag flag, signed_word_t relative_offset)
    {
        assert(relative_offset <= k_jump_max_offset && relative_offset >= k_jump_min_offset);
        assert(relative_offset % k_word_size == 0);

        // mask off extra sign bits, we'll do an arithmetic shift during decode to recover them
        word_t offset_bits = static_cast<word_t>(relative_offset / k_word_size)
                             & ((1U << k_jump_offset_encode_bits) - 1);

        return {opcode::jump, static_cast<word_t>(flag) | offset_bits << k_cmp_flag_bits};
    }

    void decode_jump(cmp_flag * flag, signed_word_t * relative_offset) const
    {
        assert(get_opcode() == opcode::jump);
        word_t tmp = storage >> k_opcode_bits;
        *flag = static_cast<cmp_flag>(tmp & ((1U << k_cmp_flag_bits) - 1));
        *relative_offset
            = (static_cast<signed_word_t>(storage) >> (k_cmp_flag_bits + k_opcode_bits))
              * k_word_size;
    }

    static instr ijump(cmp_flag flag, reg loc)
    {
        return {opcode::ijump, static_cast<word_t>(flag) | raw(loc) << k_cmp_flag_bits};
    }

    void decode_ijump(cmp_flag * flag, reg * loc) const
    {
        assert(get_opcode() == opcode::ijump);
        word_t tmp = storage >> k_opcode_bits;
        *flag = static_cast<cmp_flag>(tmp & ((1U << k_cmp_flag_bits) - 1));
        tmp >>= k_cmp_flag_bits;
        *loc = static_cast<reg>(tmp & k_reg_mask);
    }

    // we encode into this may bits
    static size_t constexpr k_call_offset_encode_bits
        = k_instr_bits - k_opcode_bits;

    // ... but users can specify this many bits, since instruction offsets must be divisible
    // by k_word_size
    static size_t constexpr k_call_offset_bits = k_call_offset_encode_bits + k_word_size_bits;

    static signed_word_t constexpr k_call_max_offset
        = k_word_size * ((1 << (k_call_offset_encode_bits - 1)) - 1);

    static signed_word_t constexpr k_call_min_offset = -k_call_max_offset;

    static instr call(signed_word_t relative_offset)
    {
        assert(relative_offset <= k_call_max_offset && relative_offset >= k_call_min_offset);
        assert(relative_offset % k_word_size == 0);

        // mask off extra sign bits, we'll do an arithmetic shift during decode to recover them
        word_t offset_bits = static_cast<word_t>(relative_offset / k_word_size)
                             & ((1U << k_call_offset_encode_bits) - 1);

        return {opcode::call, offset_bits};
    }

    void decode_call(signed_word_t * relative_offset) const
    {
        assert(get_opcode() == opcode::call);
        *relative_offset = (static_cast<signed_word_t>(storage) >> (k_opcode_bits)) * k_word_size;
    }

    word_t storage;
};

std::string_view to_str(instr::cmp_flag flag);

template <typename T>
std::optional<T> from_str(std::string_view str);

std::string to_str(instr const & ii);

template <>
struct std::formatter<instr>
{
    template <class ParseContext>
    constexpr std::format_parse_context::iterator parse(ParseContext & ctx)
    {
        return ctx.begin();
    }

    template <class FormatContext>
    auto format(instr const & ii, FormatContext & ctx) const
    {
        std::string instr = to_str(ii);
        auto it = ctx.out();
        for (char c : instr) {
            *it++ = c;
        }
        return it;
    }
};

template <>
struct std::formatter<instr::cmp_flag>
{
    template <class ParseContext>
    constexpr std::format_parse_context::iterator parse(ParseContext & ctx)
    {
        return ctx.begin();
    }

    template <class FormatContext>
    auto format(instr::cmp_flag const & flag, FormatContext & ctx) const
    {
        return std::format_to(ctx.out(), "{}", to_str(flag));
    }
};

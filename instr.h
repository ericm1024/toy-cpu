#pragma once

#include "bitfield_builder.h"
#include "cpu_base.h"
#include "opcode.h"
#include "reg.h"

#include <cassert>
#include <cstddef>
#include <format>
#include <initializer_list>
#include <optional>
#include <stdint.h>
#include <string>
#include <string_view>

static size_t constexpr k_instr_bits = sizeof(word_t) * 8;

#define ENUM_DEF_FILE_NAME "cmp_flag_def.h"
#include "enum_decl.h" // IWYU pragma: export

// Instructions are 32 bits
// First 8 bits are opcode, rest are opcode-dependent
struct instr
{
private:
    struct opcode_f : field<8, opcode>
    { };
    struct reg_f : field<k_reg_bits, reg>
    { };

    static constexpr auto base_instr_builder = bitfield_builder<word_t>().add_field<opcode_f>();

public:
    explicit instr(word_t raw)
        : storage{raw}
    { }

    instr(opcode op, word_t tail)
    {
        assert(tail < (1U << 24));
        storage = static_cast<uint8_t>(op) | (tail << k_opcode_bits);
    }

    opcode get_opcode() const
    {
        return base_instr_builder.extract<opcode_f>(storage);
    }

private:
    struct set_val_f : field<k_all_remaining_bits, word_t>
    { };

    static constexpr auto set_builder
        = base_instr_builder.add_field<reg_f>().add_field<set_val_f>();

public:
    static word_t constexpr k_max_set_value = set_builder.max_value<set_val_f>();

    static instr set(reg dest, word_t value)
    {
        return instr{set_builder.build(opcode_f{opcode::set}, reg_f{dest}, set_val_f{value})};
    }

    void decode_set(reg * dest, word_t * value) const
    {
        assert(get_opcode() == opcode::set);
        *dest = set_builder.extract<reg_f>(storage);
        *value = set_builder.extract<set_val_f>(storage);
    }

private:
    struct lhs_reg_f : reg_f
    { };

    struct rhs_reg_f : reg_f
    { };

    enum class width_sel : uint8_t
    {
    };

    struct width_sell_f : field<2, width_sel>
    { };

    static constexpr auto load_store_builder = base_instr_builder.add_field<lhs_reg_f>()
                                                   .add_field<rhs_reg_f>()
                                                   .add_field<width_sell_f>();

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
        return instr{load_store_builder.build(opcode_f{op},
                                              lhs_reg_f{addr},
                                              rhs_reg_f{src},
                                              width_sell_f{sel})};
    }

    void decode_load_store(reg * addr, reg * src, word_t * width) const
    {
        *addr = load_store_builder.extract<lhs_reg_f>(storage);
        *src = load_store_builder.extract<rhs_reg_f>(storage);
        *width = sel_to_width(load_store_builder.extract<width_sell_f>(storage));
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

    static instr add(reg dest, reg op1)
    {
        return {opcode::add,
                word_t{std::to_underlying(dest)} | (word_t{std::to_underlying(op1)} << k_reg_bits)};
    }

    void decode_add(reg * dest, reg * op1) const
    {
        assert(get_opcode() == opcode::add);
        word_t tmp = storage >> k_opcode_bits;
        *dest = static_cast<reg>(tmp & k_reg_mask);
        tmp >>= k_reg_bits;
        *op1 = static_cast<reg>(tmp & k_reg_mask);
    }

    static instr sub(reg dest, reg op1)
    {
        return {opcode::sub,
                word_t{std::to_underlying(dest)} | (word_t{std::to_underlying(op1)} << k_reg_bits)};
    }

    void decode_sub(reg * dest, reg * op1) const
    {
        assert(get_opcode() == opcode::sub);
        word_t tmp = storage >> k_opcode_bits;
        *dest = static_cast<reg>(tmp & k_reg_mask);
        tmp >>= k_reg_bits;
        *op1 = static_cast<reg>(tmp & k_reg_mask);
    }

    static instr halt()
    {
        return {opcode::halt, 0};
    }

    static instr compare(reg op1, reg op2)
    {
        return {opcode::compare,
                word_t{std::to_underlying(op1)} | (word_t{std::to_underlying(op2)} << k_reg_bits)};
    }

    void decode_compare(reg * op1, reg * op2) const
    {
        assert(get_opcode() == opcode::compare);
        word_t tmp = storage >> k_opcode_bits;
        *op1 = static_cast<reg>(tmp & k_reg_mask);
        tmp >>= k_reg_bits;
        *op2 = static_cast<reg>(tmp & k_reg_mask);
    }

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
        return {opcode::ijump,
                static_cast<word_t>(flag) | std::to_underlying(loc) << k_cmp_flag_bits};
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
    static size_t constexpr k_call_offset_encode_bits = k_instr_bits - k_opcode_bits;

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

std::string_view to_str(cmp_flag flag);

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
struct std::formatter<cmp_flag>
{
    template <class ParseContext>
    constexpr std::format_parse_context::iterator parse(ParseContext & ctx)
    {
        return ctx.begin();
    }

    template <class FormatContext>
    auto format(cmp_flag const & flag, FormatContext & ctx) const
    {
        return std::format_to(ctx.out(), "{}", to_str(flag));
    }
};

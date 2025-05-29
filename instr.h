#pragma once

#include "cpu_base.h"
#include "opcode.h"
#include "reg.h"

#include <cassert>
#include <cstddef>

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

    // value can be at most 20 bits
    static instr set(reg dest, word_t value)
    {
        assert(value < (1U << (k_instr_bits - k_opcode_bits - k_reg_bits)));
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
    static instr
    load_store(opcode op, reg addr, reg src, word_t width_sel /* 0, 1, 2 -> 1, 2, 4 bits*/)
    {
        assert(op == opcode::load || op == opcode::store);
        assert(width_sel < 3);
        return {op, raw(addr) | raw(src) << (k_reg_bits) | width_sel << (2 * k_reg_bits)};
    }

    void decode_load_store(reg * addr, reg * src, word_t * width_sel) const
    {
        word_t tmp = storage >> k_opcode_bits;
        *addr = static_cast<reg>(tmp & k_reg_mask);
        tmp >>= k_reg_bits;
        *src = static_cast<reg>(tmp & k_reg_mask);
        tmp >>= k_reg_bits;
        *width_sel = tmp & 0x3;
        assert(*width_sel < 3);
    }

public:
    static instr store(reg addr, reg src, word_t width_sel /* 0, 1, 2 -> 1, 2, 4 bits*/)
    {
        return load_store(opcode::store, addr, src, width_sel);
    }

    void decode_store(reg * addr, reg * src, word_t * width_sel) const
    {
        assert(get_opcode() == opcode::store);
        decode_load_store(addr, src, width_sel);
    }

    static instr store1(reg addr, reg src)
    {
        return store(addr, src, 0);
    }

    static instr store2(reg addr, reg src)
    {
        return store(addr, src, 1);
    }

    static instr store4(reg addr, reg src)
    {
        return store(addr, src, 2);
    }

    static instr load(reg addr, reg src, word_t width_sel /* 0, 1, 2 -> 1, 2, 4 bits*/)
    {
        return load_store(opcode::load, addr, src, width_sel);
    }

    void decode_load(reg * dest, reg * src, word_t * width_sel) const
    {
        assert(get_opcode() == opcode::load);
        decode_load_store(dest, src, width_sel);
    }

    static instr load1(reg dest, reg src)
    {
        return load(dest, src, 0);
    }

    static instr load2(reg dest, reg src)
    {
        return load(dest, src, 1);
    }

    static instr load4(reg dest, reg src)
    {
        return load(dest, src, 2);
    }

    static instr add(reg dest, reg op1, reg op2)
    {
        return {opcode::add, raw(dest) | (raw(op1) << k_reg_bits) | (raw(op2) << (2 * k_reg_bits))};
    }

    void decode_add(reg * dest, reg * op1, reg * op2)
    {
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

    word_t storage;
};

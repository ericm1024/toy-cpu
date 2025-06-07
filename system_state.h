#include "cpu_base.h"
#include "instr.h"
#include "iomap.h"
#include "reg.h"

#include <cassert>
#include <iterator>
#include <memory>
#include <span>
#include <stddef.h>
#include <stdint.h>
#include <vector>

enum class cpu_cmp_flags : uint8_t
{
    invalid = 0,
    eq = 1,
    lt = 2,
    gt = 4,
};

struct cpu
{
    cpu()
    {
        set_cmp_flag(cpu_cmp_flags::invalid);
    }

    word_t & get(reg reg)
    {
        word_t index = std::to_underlying(reg);
        assert(index < std::size(registers));
        return registers[index];
    }

    void add(reg dest, reg op1, reg op2);
    void sub(reg dest, reg op1, reg op2);

    void set_cmp_flag(cpu_cmp_flags flag)
    {
        cpu_cmp_flags = 1 << static_cast<uint8_t>(flag);
    }

    bool get_cmp_flag(cpu_cmp_flags flag) const
    {
        return cpu_cmp_flags & (1 << static_cast<uint8_t>(flag));
    }

    void jump(cmp_flag flag, signed_word_t offset);
    void ijump(cmp_flag flag, reg loc);

private:
    bool is_taken(cmp_flag flag) const;

public:
    void call(signed_word_t offset);

    uint8_t cpu_cmp_flags;
    word_t instr_ptr = iomap::k_rom_base;
    word_t registers[k_num_registers]{};
};

struct system_state
{
    system_state(std::span<word_t const> program = {});

    void set_rom(std::span<word_t const> program);
    void set_rom(std::span<uint8_t const> program);
    void set_rom(std::vector<instr> program);

private:
    void set_rom(void const * prog, size_t num_bytes);

public:
    void run();

    void execute_set(reg dest, word_t value);
    void execute_store(reg addr_reg, reg value_reg, word_t width);
    void execute_load(reg addr_reg, reg value_reg, word_t width);

    void raw_store(word_t addr, word_t value);
    word_t raw_load(word_t addr);

private:
    void execute_load_store(bool is_load, reg addr_reg, reg value_reg, word_t width);
    void execute_load_store_impl(bool is_load, word_t addr, word_t * value, word_t width);

public:
    void execute_compare(reg op1, reg op2);

    std::vector<uint8_t> console;
    std::unique_ptr<uint8_t[]> rom;
    std::unique_ptr<uint8_t[]> ram;
    cpu cpu;
};

#include "cpu_base.h"
#include "reg.h"

#include <cassert>
#include <memory>
#include <span>
#include <vector>

struct cpu
{
    word_t & get(reg reg)
    {
        word_t index = raw(reg);
        assert(index < std::size(registers));
        return registers[index];
    }

    word_t registers[raw(reg::num_registers)]{};
};

struct system_state
{
    system_state(std::span<word_t const> program = {});

    void set_rom(std::span<word_t const> program);
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
    void execute_add(reg dest, reg op1, reg op2);

    std::vector<uint8_t> console;
    std::unique_ptr<uint8_t[]> rom;
    std::unique_ptr<uint8_t[]> ram;
    cpu cpu;
};

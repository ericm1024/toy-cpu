#include "assembler.h"
#include "instr.h"
#include "iomap.h"
#include "system_state.h"
#include "test.h"

#include <cassert>
#include <vector>

static std::vector<word_t> make_hello_world_rom()
{
    std::vector<word_t> rom;
    char const * str = "Hello world!";
    rom.push_back(instr::set(r0, iomap::k_console_write).storage);
    for (size_t i = 0; i < strlen(str); ++i) {
        rom.push_back(instr::set(r1, str[i]).storage);
        rom.push_back(instr::store1(r0, r1).storage);
    }
    rom.push_back(instr::halt().storage);
    return rom;
}

TEST("full_program.hello_world")
{
    system_state system{make_hello_world_rom()};
    system.run();
    system.console.push_back('\0');
    assert(strcmp((char const *)system.console.data(), "Hello world!") == 0);
}

// reads 42 and 43 from rom, adds them writes the result to ram
static std::vector<word_t> make_add_numbers_rom()
{
    std::vector<word_t> rom;
    rom.resize(128);

    word_t const k_nums_offset = 64 * k_word_size;
    rom[k_nums_offset / k_word_size] = 42;
    rom[k_nums_offset / k_word_size + 1] = 43;

    // r0 <- k_rom_base + k_nums_offset
    size_t i_instr = 0;
    rom[i_instr++] = instr::set(r0, iomap::k_rom_base + k_nums_offset).storage;

    // r1 <- *r0     ;; r1 holds 42
    rom[i_instr++] = instr::load4(r1, r0).storage;

    // r2 = 4
    rom[i_instr++] = instr::set(r2, 4).storage;

    // r0 = r0 + r2
    rom[i_instr++] = instr::add(r0, r0, r2).storage;

    // r3 <- *r0       ;; r3 holds 43
    rom[i_instr++] = instr::load4(r3, r0).storage;

    // r1 <- r1 + r3   ;; r1 = 42 + 43
    rom[i_instr++] = instr::add(r1, r1, r3).storage;

    // r0 = k_ram_base
    rom[i_instr++] = instr::set(r0, iomap::k_ram_base).storage;

    // *r0 = r1
    rom[i_instr++] = instr::store4(r0, r1).storage;

    rom[i_instr++] = instr::halt().storage;
    return rom;
}

TEST("full_program.add_numbers")
{
    system_state system{make_add_numbers_rom()};
    system.run();
    assert(system.raw_load(iomap::k_ram_base) == 42 + 43);
}

static std::vector<uint8_t> make_fib_rom()
{
/*
int fib(int x)
{
    if (x <= 1) {
        return 1;
    }
    return fib(x - 1) + fib(x - 2);
*/

    std::string prog = std::format(R"(

    # initialize stack pointer to k_ram_base
    set r0 {}

# stack pointer is in r0, return value is in r1, argument is in r2
fib:
    set r3 1
    compare r2 r3
    branch.gt recurse # r2 > 1, recurse

recurse:

)", iomap::k_ram_base);

    return assemble(prog);
}

TEST("full_program.fib")
{
    std::vector<uint8_t> fib_rom = make_fib_rom();
}

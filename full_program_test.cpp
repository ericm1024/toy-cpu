#include "assembler.h"
#include "cpu_base.h"
#include "instr.h"
#include "iomap.h"
#include "log.h"
#include "reg.h"
#include "system_state.h"
#include "test.h"

#include <cassert>
#include <cstring>
#include <format>
#include <initializer_list>
#include <span>
#include <stdint.h>
#include <string>
#include <vector>

static logger logger{__FILE__};

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
    std::string prog = std::format(R"(

    # initialize stack pointer to k_ram_base
    set r14 {}

    # test sets the function argument in r0

    # call fib (sets r15 to return address)
    call fib
    halt

# argument is in r0, return value is in r13, stack pointer r14, return address r15
# callee r15
fib:
    set r1 1
    compare r0 r1     # compare x with 1
    jump.gt recurse   # x > 1, recursive case
    set r13 1         # set return value to 1
    ijump r15         # return

recurse:
    set r2 4

    store r14 r15     # store r15 (return address) onto the stack
    add r14 r14 r2    # increment stack pointer

    store r14 r0      # store r0 (function argument) onto the stack
    add r14 r14 r2    # increment stack pointer

    set r2 1
    sub r0 r0 r2      # compute x = x - 1

    call fib          # recurse, calling fib(x-1)

    set r2 4
    sub r14 r14 r2    # decrement stack pointer
    load r0 r14       # pop r0 (function argument) off the stack

    store r14 r13     # store return value from fib(x-1) on the stack
    add r14 r14 r2    # increment stack pointer

    set r2 2
    sub r0 r0 r2      # compute x = x - 2

    call fib          # recurse, calling fib(x-2)

    set r2 4
    sub r14 r14 r2    # decrement stack pointer
    load r1 r14       # pop fib(x-1) into r1

    add r13 r13 r1    # set ret = fib(x-2) + fib(x-1)

    sub r14 r14 r2    # decrement stack pointer
    load r15 r14      # pop return address off the stack
    ijump r15         # return

)",
                                   iomap::k_ram_base);

    return assemble(prog);
}

static word_t fib(word_t x)
{
    if (x <= 1) {
        return 1;
    }
    return fib(x - 1) + fib(x - 2);
}

TEST("full_program.fib")
{
    std::vector<uint8_t> fib_rom = make_fib_rom();
    for (word_t i : {1, 2, 3, 4, 5}) {
        word_t expected = fib(i);
        logger.debug("computing fib({}), expecting {}", i, expected);
        system_state system;
        system.set_rom(fib_rom);
        system.cpu.get(r0) = i;
        system.run();
        assert(system.cpu.get(r13) == expected);
    }
}

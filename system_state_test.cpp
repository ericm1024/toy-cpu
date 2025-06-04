#include "assembler.h"
#include "instr.h"
#include "iomap.h"
#include "system_state.h"
#include "test.h"

#include <cassert>
#include <utility>

TEST("system_state.execute.load_store")
{
    // 4-byte read/write from RAM
    {
        word_t const val = 2348976123;
        system_state state{};
        state.cpu.get(r0) = iomap::k_ram_base;
        state.cpu.get(r1) = val;
        state.set_rom({
            instr::store4(r0, r1),
            instr::load4(r2, r0),
            instr::halt(),
        });
        state.run();
        assert(state.cpu.get(r2) == val);
    }

    // 2-byte read/write from RAM
    {
        word_t const val = 7287;
        system_state state{};
        state.cpu.get(r0) = iomap::k_ram_base + 2;
        state.cpu.get(r1) = val;
        state.set_rom({
            instr::store2(r0, r1),
            instr::load2(r2, r0),
            instr::halt(),
        });
        state.run();
        assert(state.cpu.get(r2) == val);
    }

    // 1-byte read/write from RAM
    {
        word_t const val = 209;
        system_state state{};
        state.cpu.get(r0) = iomap::k_ram_base + 1;
        state.cpu.get(r1) = val;
        state.set_rom({
            instr::store1(r0, r1),
            instr::load1(r2, r0),
            instr::halt(),
        });
        state.run();
        assert(state.cpu.get(r2) == val);
    }
}

TEST("system_state.execute.console")
{
    system_state state{};
    state.set_rom({
        instr::set(r0, 'a'),
        instr::set(r1, iomap::k_console_write),
        instr::store1(r1, r0),
        instr::set(r0, 'b'),
        instr::store1(r1, r0),
        instr::set(r0, 'c'),
        instr::store1(r1, r0),
        instr::halt(),
    });
    state.run();
    assert((state.console == std::vector<uint8_t>{'a', 'b', 'c'}));
}

TEST("system_state.execute.set")
{
    system_state state{};
    word_t const val = 12345;
    state.set_rom({
        instr::set(r15, val),
        instr::halt(),
    });
    state.run();
    assert(state.cpu.get(r15) == val);
}

TEST("system_state.execute.add")
{
    system_state state{};
    state.set_rom({
        instr::set(r0, 12345),
        instr::set(r1, 98678),
        instr::add(r2, r1, r0),
        instr::halt(),
    });
    state.run();
    assert(state.cpu.get(r2) == 12345 + 98678);
}

TEST("system_state.execute.sub")
{
    system_state state{};
    state.set_rom({
        instr::set(r0, 12345),
        instr::set(r1, 98678),
        instr::sub(r2, r1, r0),
        instr::halt(),
    });
    state.run();
    assert(state.cpu.get(r2) == word_t{98678} - word_t{12345});
}

TEST("system_state.execute.jump")
{
    struct
    {
        instr::cmp_flag flag;
        std::pair<word_t, word_t> comparison;
        bool taken;
    } cases[] = {
        {instr::cmp_flag::eq, {1, 1}, true},
        {instr::cmp_flag::eq, {1, 0}, false},

        {instr::cmp_flag::ne, {1, 0}, true},
        {instr::cmp_flag::ne, {1, 1}, false},

        {instr::cmp_flag::gt, {1, 0}, true},
        {instr::cmp_flag::gt, {1, 1}, false},
        {instr::cmp_flag::gt, {1, 2}, false},

        {instr::cmp_flag::ge, {1, 0}, true},
        {instr::cmp_flag::ge, {1, 1}, true},
        {instr::cmp_flag::ge, {1, 2}, false},

        {instr::cmp_flag::lt, {1, 0}, false},
        {instr::cmp_flag::lt, {1, 1}, false},
        {instr::cmp_flag::lt, {1, 2}, true},

        {instr::cmp_flag::le, {1, 0}, false},
        {instr::cmp_flag::le, {1, 1}, true},
        {instr::cmp_flag::le, {1, 2}, true},
    };

    for (auto [flag, comparison, taken] : cases) {
        system_state state{};
        state.set_rom({
            instr::set(r0, comparison.first),
            instr::set(r1, comparison.second),
            instr::compare(r0, r1),
            instr::jump(flag, k_word_size * 3),
            instr::set(r2, 42),
            instr::halt(),
            instr::set(r3, 42),
            instr::halt(),
        });
        state.run();
        if (taken) {
            assert(state.cpu.get(r2) == 0);
            assert(state.cpu.get(r3) == 42);
        } else {
            assert(state.cpu.get(r2) == 42);
            assert(state.cpu.get(r3) == 0);
        }
    }
}

// Simple for-loop incrementing r0 until it reaches the value 5. Exercises a backwards jump.
TEST("system_state.execute.jump.backwards")
{
    char const * prog = R"(
set r1 5
set r2 1
set r0 0
add r0 r0 r2
compare r0 r1
jump.ne -8
halt
)";
    std::vector<uint8_t> rom = assemble(prog);
    system_state state{};
    state.set_rom(rom);
    state.run();
    assert(state.cpu.get(r0) == 5);
}

TEST("system_state.call")
{
    char const * prog = R"(
set r1 5
set r2 7
call my_func
halt
my_func:
add r0 r1 r2
ijump r15
)";
    std::vector<uint8_t> rom = assemble(prog);
    system_state state{};
    state.set_rom(rom);
    state.run();
    assert(state.cpu.get(r0) == 12);
}

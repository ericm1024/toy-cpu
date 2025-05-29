#include "instr.h"
#include "iomap.h"
#include "system_state.h"
#include "test.h"

#include <cassert>

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

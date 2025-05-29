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

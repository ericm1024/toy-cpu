#include "assembler.h"
#include "cpu_base.h"
#include "instr.h"
#include "test.h"

#include <span>
#include <vector>

static void verify_rom(std::span<uint8_t const> rom, std::initializer_list<instr> instructions)
{
    assert(rom.size() == instructions.size() * sizeof(word_t));
    auto it = rom.begin();
    for (instr ii : instructions) {
        assert(memcmp(&ii.storage, &*it, sizeof(word_t)) == 0);
        it += sizeof(word_t);
    }
}

TEST("assembler.basic")
{
    std::vector<uint8_t> rom = assemble("set r1 100\nset r2 42");
    verify_rom(rom, {instr::set(r1, 100), instr::set(r2, 42)});
}

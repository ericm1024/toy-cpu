#include "assembler.h"
#include "cpu_base.h"
#include "instr.h"
#include "test.h"

#include <span>
#include <sstream>
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

TEST("assembler.load_store")
{
    std::pair<char const *, instr (*)(reg, reg, word_t)> const mnemonic_table[] = {
        {"load", &instr::load},
        {"store", &instr::store},
    };

    for (auto & [mnemonic, ctor] : mnemonic_table) {
        for (reg src : {reg::r1, reg::r2}) {
            for (reg dst : {reg::r3, reg::r4}) {
                for (word_t width : {0, 1, 2, 4}) {
                    std::stringstream ss;
                    ss << mnemonic;
                    if (width != 0) {
                        ss << ".";
                        ss << width;
                    }

                    ss << " ";
                    ss << to_str(src);

                    ss << " ";
                    ss << to_str(dst);

                    word_t width_sel = width == 0 ? 2 : width == 1 ? 0 : width == 2 ? 1 : 2;
                    instr ii = ctor(src, dst, width_sel);

                    std::vector<uint8_t> rom = assemble(ss.str());
                    verify_rom(rom, {ii});
                }
            }
        }
    }
}

TEST("assembler.add")
{
    for (reg dst : {reg::r1, reg::r2}) {
        for (reg op1 : {reg::r3, reg::r4}) {
            for (reg op2 : {reg::r5, reg::r6}) {
                std::stringstream ss;
                ss << "add";

                ss << " ";
                ss << to_str(dst);

                ss << " ";
                ss << to_str(op1);

                ss << " ";
                ss << to_str(op2);

                std::vector<uint8_t> rom = assemble(ss.str());
                verify_rom(rom, {instr::add(dst, op1, op2)});
            }
        }
    }
}

TEST("assembler.halt")
{
    std::vector<uint8_t> rom = assemble("halt");
    verify_rom(rom, {instr::halt()});
}

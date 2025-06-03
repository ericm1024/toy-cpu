#include "assembler.h"
#include "cpu_base.h"
#include "instr.h"
#include "test.h"

#include <span>
#include <sstream>
#include <vector>

static void do_test(std::string program, std::initializer_list<instr> instructions)
{
    std::vector<uint8_t> rom = assemble(program);
    auto it = rom.begin();
    for (instr ii : instructions) {
        assert(memcmp(&ii.storage, &*it, sizeof(word_t)) == 0);
        it += sizeof(word_t);
    }
    std::string disassembly = disassemble(rom);

    // we can't compare the disassembly with the original program because the mapping from text
    // to bytecode is not 1-to-1 (e.g. load.4 is equivalent to load, jump labels & comments are
    // lost, etc)

    assert(assemble(disassembly) == rom);
}

TEST("assembler.basic")
{
    do_test("set r1 100\nset r2 42", {instr::set(r1, 100), instr::set(r2, 42)});
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

                    instr ii = ctor(src, dst, width != 0 ? width : k_word_size);

                    do_test(ss.str(), {ii});
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

                do_test(ss.str(), {instr::add(dst, op1, op2)});
            }
        }
    }
}

TEST("assembler.halt")
{
    do_test("halt", {instr::halt()});
}

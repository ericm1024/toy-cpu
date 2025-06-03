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
    // to bytecode is not 1-to-1 (e.g. load.4 is equivalent to load, ijump labels & comments are
    // lost, etc)

    assert(assemble(disassembly) == rom);
}

TEST("assembler.basic")
{
    do_test("set r1 100\nset r2 42", {instr::set(r1, 100), instr::set(r2, 42)});
}

TEST("assembler.blank_lines")
{
    do_test("\nset r1 100\n\n\nset r2 42", {instr::set(r1, 100), instr::set(r2, 42)});
}

TEST("assembler.comments")
{
    do_test(R"(
# a comment
    # another comment

set r1 100 # a comment after an instruction
set r2 42
# yet another
)",
            {instr::set(r1, 100), instr::set(r2, 42)});
}

TEST("assembler.load_store")
{
    std::pair<char const *, instr (*)(reg, reg, word_t)> const mnemonic_table[] = {
        {"load", &instr::load},
        {"store", &instr::store},
    };

    for (auto & [mnemonic, ctor] : mnemonic_table) {
        for (reg src : k_all_registers) {
            for (reg dst : k_all_registers) {
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
    for (reg dst : k_all_registers) {
        for (reg op1 : k_all_registers) {
            for (reg op2 : k_all_registers) {
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

TEST("assembler.cmp")
{
    for (reg op1 : k_all_registers) {
        for (reg op2 : k_all_registers) {
            std::stringstream ss;
            ss << "compare";

            ss << " ";
            ss << to_str(op1);

            ss << " ";
            ss << to_str(op2);
            do_test(ss.str(), {instr::compare(op1, op2)});
        }
    }
}

static void test_jump(signed_word_t offset, instr::cmp_flag flag, bool adorn = true)
{
    std::stringstream ss;
    ss << "jump";
    if (adorn) {
        ss << ".";
        ss << to_str(flag);
    }

    ss << " ";
    ss << offset;

    do_test(ss.str(), {instr::jump(flag, offset)});
}

TEST("assembler.jump")
{
    for (signed_word_t offset : {instr::k_jump_min_offset, -4, 0, 4, instr::k_jump_max_offset}) {
        for (instr::cmp_flag flag : instr::k_all_cmp_flags) {
            test_jump(offset, flag);
        }

        // test unadored unconditional jump
        test_jump(offset, instr::unc, false /* !adorn */);
    }
}

TEST("assembler.jump.labels")
{
    // forward label
    do_test(
        R"(
compare r0 r1
jump.eq label
set r1 15
label:
halt
)",
        {instr::compare(r0, r1), instr::jump(instr::eq, 8), instr::set(r1, 15), instr::halt()});

    // backwards label. This is obviously a infinite loop, but it tests the assembler
    do_test(R"(
label:
compare r0 r1
jump.eq label
)",
            {instr::compare(r0, r1), instr::jump(instr::eq, -4)});
}

static void test_ijump(reg loc, instr::cmp_flag flag, bool adorn = true)
{
    std::stringstream ss;
    ss << "ijump";
    if (adorn) {
        ss << ".";
        ss << to_str(flag);
    }

    ss << " ";
    ss << to_str(loc);

    do_test(ss.str(), {instr::ijump(flag, loc)});
}

TEST("assembler.ijump")
{
    for (reg loc : k_all_registers) {
        for (instr::cmp_flag flag : instr::k_all_cmp_flags) {
            test_ijump(loc, flag);
        }

        // test unadored unconditional ijump
        test_ijump(loc, instr::unc, false /* !adorn */);
    }
}

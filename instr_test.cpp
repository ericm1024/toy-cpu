#include "instr.h"
#include "log.h"
#include "test.h"

#include <cassert>
#include <initializer_list>

static logger logger;

TEST("instr.enc_dec_set")
{
    for (reg rr : k_all_registers) {
        for (word_t value :
             {0U,
              std::uniform_int_distribution{1U, instr::k_max_set_value - 1}(test_rng()),
              instr::k_max_set_value}) {
            instr ii = instr::set(rr, value);

            reg rr_out;
            word_t value_out;
            ii.decode_set(&rr_out, &value_out);
            assert(rr_out == rr);
            assert(value_out == value);
        }
    }
}

TEST("instr.enc_dec_add")
{
    for (reg dest : k_all_registers) {
        for (reg op1 : k_all_registers) {
            for (reg op2 : k_all_registers) {
                instr ii = instr::add(dest, op1, op2);
                reg dest_out, op1_out, op2_out;
                ii.decode_add(&dest_out, &op1_out, &op2_out);
                assert(dest_out == dest);
                assert(op1_out == op1);
                assert(op2_out == op2);
            }
        }
    }
}

TEST("instr.enc_dec_load_store")
{
    for (reg lhs : k_all_registers) {
        for (reg rhs : k_all_registers) {
            for (word_t width : {1, 2, 4}) {
                instr ii = instr::load(lhs, rhs, width);
                reg lhs_out, rhs_out;
                word_t width_out;
                ii.decode_load(&lhs_out, &rhs_out, &width_out);
                assert(lhs_out == lhs);
                assert(rhs_out == rhs);
                assert(width_out == width);

                ii = instr::store(lhs, rhs, width);
                ii.decode_store(&lhs_out, &rhs_out, &width_out);
                assert(lhs_out == lhs);
                assert(rhs_out == rhs);
                assert(width_out == width);
            }
        }
    }
}

TEST("instr.compare")
{
    for (reg lhs : k_all_registers) {
        for (reg rhs : k_all_registers) {
            instr ii = instr::compare(lhs, rhs);
            reg lhs_out, rhs_out;
            ii.decode_compare(&lhs_out, &rhs_out);
            assert(lhs_out == lhs);
            assert(rhs_out == rhs);
        }
    }
}

TEST("instr.branch")
{
    for (instr::cmp_flag flag :
         {instr::eq, instr::ne, instr::gt, instr::ge, instr::lt, instr::le}) {
        for (signed_word_t offset :
             {instr::k_branch_min_offset, -4, 0, 4, instr::k_branch_max_offset}) {
            instr ii = instr::branch(flag, offset);
            instr::cmp_flag flag_out;
            signed_word_t offset_out;
            ii.decode_branch(&flag_out, &offset_out);

            assert(flag_out == flag);
            assert(offset_out == offset);
        }
    }
}

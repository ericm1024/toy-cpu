#include "instr.h"
#include "test.h"

#include <cassert>
#include <initializer_list>

TEST("instr.enc_dec_set")
{
    instr ii = instr::set(r1, 42);
    reg dest;
    word_t value;
    ii.decode_set(&dest, &value);
    assert(dest == r1);
    assert(value == 42);
}

TEST("instr.enc_dec_add")
{
    instr ii = instr::add(r1, r2, r3);
    reg dest, op1, op2;
    ii.decode_add(&dest, &op1, &op2);
    assert(dest == r1);
    assert(op1 == r2);
    assert(op2 == r3);
}

TEST("instr.enc_dec_load_store")
{
    for (word_t width_sel : {0, 1, 2}) {
        instr ii = instr::load(r4, r3, width_sel);
        reg addr, src;
        word_t width_sel_out;
        ii.decode_load(&addr, &src, &width_sel_out);
        assert(addr == r4);
        assert(src == r3);
        assert(width_sel_out == width_sel);

        ii = instr::store(r5, r6, width_sel);
        ii.decode_store(&addr, &src, &width_sel_out);
        assert(addr == r5);
        assert(src == r6);
        assert(width_sel_out == width_sel);
    }
}

#include "instr.h"

#include <iterator>

#define ENUM_DEF_FILE_NAME "cmp_flag_def.h"
#include "enum_def.h"

std::initializer_list<cmp_flag> const instr::k_all_cmp_flags{
#define X(x) x,
#include "cmp_flag_def.h"
#undef X
};

std::string to_str(instr const & ii)
{
    switch (ii.get_opcode()) {
    case opcode::set: {
        reg dest;
        word_t value;
        ii.decode_set(&dest, &value);
        return std::format("set {} {}", dest, value);
    }
    case opcode::store: {
        reg addr, src;
        word_t width;
        ii.decode_store(&addr, &src, &width);
        return std::format("store.{} {} {}", width, addr, src);
    }
    case opcode::load: {
        reg dest, addr;
        word_t width;
        ii.decode_load(&dest, &addr, &width);
        return std::format("load.{} {} {}", width, dest, addr);
    }
    case opcode::add: {
        reg dest, op1, op2;
        ii.decode_add(&dest, &op1, &op2);
        return std::format("add {} {} {}", dest, op1, op2);
    }
    case opcode::sub: {
        reg dest, op1, op2;
        ii.decode_sub(&dest, &op1, &op2);
        return std::format("sub {} {} {}", dest, op1, op2);
    }
    case opcode::halt:
        return "halt";
    case opcode::compare: {
        reg op1, op2;
        ii.decode_compare(&op1, &op2);
        return std::format("compare {} {}", op1, op2);
    }
    case opcode::jump: {
        cmp_flag flag;
        signed_word_t relative_offset;
        ii.decode_jump(&flag, &relative_offset);
        return std::format("jump.{} {}", to_str(flag), relative_offset);
    }
    case opcode::ijump: {
        cmp_flag flag;
        reg loc;
        ii.decode_ijump(&flag, &loc);
        return std::format("ijump.{} {}", flag, loc);
    }
    case opcode::call: {
        signed_word_t relative_offset;
        ii.decode_call(&relative_offset);
        return std::format("call {}", relative_offset);
    }
    default:
        return "unknown";
    }
}

#include "instr.h"

static std::string_view const cmp_flag_to_str[] = {
#define MAKE_ENTRY(flag)                                                                           \
    [static_cast<uint8_t>(instr::cmp_flag::flag)] = std::string_view                               \
    {                                                                                              \
        #flag                                                                                      \
    }

    MAKE_ENTRY(eq),
    MAKE_ENTRY(ne),
    MAKE_ENTRY(gt),
    MAKE_ENTRY(ge),
    MAKE_ENTRY(lt),
    MAKE_ENTRY(le),
    MAKE_ENTRY(unc),
};

std::initializer_list<instr::cmp_flag> const instr::k_all_cmp_flags{
    instr::eq,
    instr::ne,
    instr::gt,
    instr::ge,
    instr::lt,
    instr::le,
    instr::unc,
};

std::string_view to_str(instr::cmp_flag flag)
{
    word_t index = static_cast<uint8_t>(flag);
    if (index >= std::size(cmp_flag_to_str)) {
        return "unknown";
    }
    return cmp_flag_to_str[index];
}

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
    case opcode::halt:
        return "halt";
    case opcode::compare: {
        reg op1, op2;
        ii.decode_compare(&op1, &op2);
        return std::format("compare {} {}", op1, op2);
    }
    case opcode::jump: {
        instr::cmp_flag flag;
        signed_word_t relative_offset;
        ii.decode_jump(&flag, &relative_offset);
        return std::format("jump.{} {}", to_str(flag), relative_offset);
    }
    case opcode::ijump: {
        instr::cmp_flag flag;
        reg loc;
        ii.decode_ijump(&flag, &loc);
        return std::format("ijump.{} {}", flag, loc);
    }
    default:
        return "unknown";
    }
}

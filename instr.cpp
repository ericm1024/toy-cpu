#include "instr.h"

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
    default:
        return "unknown";
    }
}

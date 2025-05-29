#include "opcode.h"

#include <iterator>

static char const * const opcode_to_str[] = {
#define MAKE_ENTRY(op) [raw(opcode::op)] = #op

    MAKE_ENTRY(set),
    MAKE_ENTRY(store),
    MAKE_ENTRY(add),
    MAKE_ENTRY(halt),
    MAKE_ENTRY(load),
};

char const * to_str(opcode op)
{
    word_t index = raw(op);
    if (index >= std::size(opcode_to_str)) {
        return "unknown";
    }
    return opcode_to_str[index];
}

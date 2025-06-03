#include "opcode.h"

#include <iterator>
#include <optional>
#include <string_view>

static std::string_view const opcode_to_str[] = {
#define MAKE_ENTRY(op)                                                                             \
    [raw(opcode::op)] = std::string_view                                                           \
    {                                                                                              \
        #op                                                                                        \
    }

    MAKE_ENTRY(set),
    MAKE_ENTRY(store),
    MAKE_ENTRY(add),
    MAKE_ENTRY(halt),
    MAKE_ENTRY(load),
    MAKE_ENTRY(compare),
    MAKE_ENTRY(branch),
    MAKE_ENTRY(jump),
};

template <>
std::optional<opcode> from_str<opcode>(std::string_view str)
{
    for (size_t i = 0; i < std::size(opcode_to_str); ++i) {
        if (opcode_to_str[i] == str) {
            return static_cast<opcode>(i);
        }
    }
    return std::nullopt;
}

char const * to_str(opcode op)
{
    word_t index = raw(op);
    if (index >= std::size(opcode_to_str)) {
        return "unknown";
    }
    return opcode_to_str[index].data();
}

#ifndef ENUM_DEF_FILE_NAME
#error "must define ENUM_DEF_FILE_NAME"
#endif

#include "preprocessor.h"

#include <iterator>
#include <optional>
#include <string_view>

#define X(arg)
#include ENUM_DEF_FILE_NAME
#undef X

#define ENUM_TABLE_NAME PASTE(ENUM_TYPE_NAME, _enum_table)

static std::string_view const ENUM_TABLE_NAME[] = {
#define X(op)                                                           \
    [raw(ENUM_TYPE_NAME::op)] = std::string_view                                \
    {                                                                   \
        #op                                                             \
    },
#include ENUM_DEF_FILE_NAME
#undef X
};

template <>
std::optional<ENUM_TYPE_NAME> from_str<ENUM_TYPE_NAME>(std::string_view str)
{
    for (size_t i = 0; i < std::size(ENUM_TABLE_NAME); ++i) {
        if (ENUM_TABLE_NAME[i] == str) {
            return static_cast<ENUM_TYPE_NAME>(i);
        }
    }
    return std::nullopt;
}

std::string_view to_str(ENUM_TYPE_NAME op)
{
    auto index = raw(op);
    if (index >= std::size(ENUM_TABLE_NAME)) {
        return "unknown";
    }
    return ENUM_TABLE_NAME[index];
}

#undef ENUM_TABLE_NAME
#undef ENUM_TYPE_NAME
#undef ENUM_UNDERLYING_TYPE
#undef ENUM_DEF_FILE_NAME

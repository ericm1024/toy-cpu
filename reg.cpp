#include "reg.h"

#include <iterator>
#include <string>

static std::string_view const reg_to_str[] = {
#define MAKE_ENTRY(r)                                                                              \
    [raw(reg::r)] = std::string_view                                                               \
    {                                                                                              \
        #r                                                                                         \
    }

    MAKE_ENTRY(r0),
    MAKE_ENTRY(r1),
    MAKE_ENTRY(r2),
    MAKE_ENTRY(r3),
    MAKE_ENTRY(r4),
    MAKE_ENTRY(r5),
    MAKE_ENTRY(r6),
    MAKE_ENTRY(r7),
    MAKE_ENTRY(r8),
    MAKE_ENTRY(r9),
    MAKE_ENTRY(r10),
    MAKE_ENTRY(r11),
    MAKE_ENTRY(r12),
    MAKE_ENTRY(r13),
    MAKE_ENTRY(r14),
    MAKE_ENTRY(r15),
};

reg const k_all_registers[raw(reg::num_registers)] = {
    r0,
    r1,
    r2,
    r3,
    r4,
    r5,
    r6,
    r7,
    r8,
    r9,
    r10,
    r11,
    r12,
    r13,
    r14,
    r15,
};

template <>
std::optional<reg> from_str<reg>(std::string_view str)
{
    for (size_t i = 0; i < std::size(reg_to_str); ++i) {
        if (reg_to_str[i] == str) {
            return static_cast<reg>(i);
        }
    }
    return std::nullopt;
}

std::string_view to_str(reg rr)
{
    uint8_t index = raw(rr);
    if (index >= std::size(reg_to_str)) {
        return "unknown";
    }
    return reg_to_str[index];
}

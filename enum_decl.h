#include <optional>
#include <string_view>

#ifndef ENUM_DEF_FILE_NAME
#error "must define ENUM_DEF_FILE_NAME"
#endif

#define X(arg)
#include ENUM_DEF_FILE_NAME
#undef X

enum class ENUM_TYPE_NAME : ENUM_UNDERLYING_TYPE
{
#define X(arg) arg,
#include ENUM_DEF_FILE_NAME
#undef X
};

static constexpr ENUM_UNDERLYING_TYPE raw(ENUM_TYPE_NAME value)
{
    return static_cast<ENUM_UNDERLYING_TYPE>(value);
}

std::string_view to_str(ENUM_TYPE_NAME);

template <typename T>
std::optional<T> from_str(std::string_view str);
template <>
std::optional<ENUM_TYPE_NAME> from_str<ENUM_TYPE_NAME>(std::string_view str);

#undef ENUM_TYPE_NAME
#undef ENUM_UNDERLYING_TYPE
#undef ENUM_DEF_FILE_NAME

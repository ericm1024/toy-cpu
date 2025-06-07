#pragma once

#include <concepts>
#include <cstdint>
#include <type_traits>

// TODO:
// * bounds checking in build
// * "all remaining bits"-sized fields
// * expose min/max values of fields

template <size_t width_in, typename repr_t>
struct field
{
    static size_t constexpr width = width_in;
    using repr_type = repr_t;

    repr_type repr;
};

template <typename field_t>
struct field_info
{
    using field = field_t;
    uint8_t start = 0;
};

template <std::integral storage_t, typename... field_ts>
struct bitfield_builder : private field_info<field_ts>...
{
    template <typename field_t>
    consteval auto add_field() const
    {
        bitfield_builder<storage_t, field_ts..., field_t> ret;
        (void(static_cast<field_info<field_ts> &>(ret)
              = static_cast<field_info<field_ts> const &>(*this)),
         ...);
        static_cast<field_info<field_t> &>(ret).start
            = (field_info<field_ts>::field::width + ... + 0);
        return ret;
    }

    storage_t build(field_ts... fields) const
    {
        return ((static_cast<storage_t>(fields.repr)
                 << static_cast<field_info<field_ts> const &>(*this).start)
                | ...);
    }

    template <typename field_t>
    typename field_t::repr_type extract(storage_t val) const
    {
        static_assert((std::is_same_v<field_t, field_ts> || ...));
        auto & info = static_cast<field_info<field_t> const &>(*this);
        storage_t mask = (storage_t{1} << field_t::width) - 1;
        return static_cast<typename field_t::repr_type>((val >> info.start) & mask);
    }

private:
    template <std::integral, typename...>
    friend struct bitfield_builder;
};

#pragma once

#include <cassert>
#include <concepts>
#include <cstdint>
#include <limits>
#include <type_traits>
#include <utility>

// TODO:
// * support for signed types
// * this-deduction for get_info

size_t constexpr k_all_remaining_bits = std::numeric_limits<size_t>::max();

template <size_t width_in, typename repr_t>
struct field
{
    static size_t constexpr width_spec = width_in;
    using repr_type = repr_t;

    repr_type value;
};

template <typename field_t>
struct field_info
{
    using field = field_t;
    uint8_t start = 0;
    uint8_t width = 0;

    constexpr auto max_value() const
    {
        size_t unused_bits = sizeof(uintmax_t) * 8 - width;
        return ~uintmax_t{0} >> unused_bits;
    }
};

template <std::unsigned_integral storage_t, typename... field_ts>
struct bitfield_builder : private field_info<field_ts>...
{
    template <typename field_t>
    constexpr auto add_field() const
    {
        uint8_t used_bits = get_used_bits();
        uint8_t availible_bits = sizeof(storage_t) * 8 - used_bits;

        // XXX why can't I do this in a constexpr-friendly way
        // static_assert(availible_bits != 0 && field_t::width_spec != 0
        //              && (field_t::width_spec <= availible_bits
        //                  || field_t::width_spec == k_all_remaining_bits));

        bitfield_builder<storage_t, field_ts..., field_t> ret;
        (void(ret.template get_info<field_ts>() = this->template get_info<field_ts>()), ...);

        auto & new_spec = ret.template get_info<field_t>();
        new_spec.start = used_bits;
        new_spec.width
            = field_t::width_spec == k_all_remaining_bits ? availible_bits : field_t::width_spec;
        return ret;
    }

    constexpr storage_t build(field_ts... fields) const
    {
        return (build_one_field<field_ts>(fields) | ...);
    }

    template <typename field_t>
    constexpr auto extract(storage_t val) const
    {
        auto & info = get_info<field_t>();
        storage_t mask = (storage_t{1} << info.width) - 1;
        return static_cast<typename field_t::repr_type>((val >> info.start) & mask);
    }

    template <typename field_t>
    constexpr auto max_value() const
    {
        return static_cast<typename field_t::repr_type>(get_info<field_t>().max_value());
    }

private:
    template <typename field_t>
    constexpr storage_t build_one_field(field_t field) const
    {
        auto & info = get_info<field_t>();
        assert(static_cast<uintmax_t>(field.value) <= std::numeric_limits<storage_t>::max());
        assert(static_cast<uintmax_t>(field.value) <= info.max_value());

        return static_cast<storage_t>(field.value) << info.start;
    }

    template <typename field_t>
    constexpr auto const & get_info() const
    {
        return static_cast<field_info<field_t> const &>(*this);
    }

    template <typename field_t>
    constexpr auto & get_info()
    {
        return static_cast<field_info<field_t> &>(*this);
    }

    constexpr uint8_t get_used_bits() const
    {
        return (get_info<field_ts>().width + ... + 0);
    }

    template <std::unsigned_integral, typename...>
    friend struct bitfield_builder;
};

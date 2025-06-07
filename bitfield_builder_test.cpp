#include "bitfield_builder.h"
#include "test.h"

TEST("bitfield_builder.max_value")
{
    struct my_field_1 : field<8, uint32_t>
    { };
    struct my_field_2 : field<3, uint32_t>
    { };
    struct my_field_3 : field<k_all_remaining_bits, uint32_t>
    { };

    {
        auto builder = bitfield_builder<uint32_t>()
                           .add_field<my_field_1>()
                           .add_field<my_field_2>()
                           .add_field<my_field_3>();
        assert(builder.max_value<my_field_3>() == ((1 << 21) - 1));
    }

    {
        auto builder = bitfield_builder<uint32_t>().add_field<my_field_3>();
        assert(builder.max_value<my_field_3>() == std::numeric_limits<uint32_t>::max());
    }

    struct my_field_uintmax : field<k_all_remaining_bits, uintmax_t>
    { };

    {
        auto builder = bitfield_builder<uintmax_t>().add_field<my_field_uintmax>();
        assert(builder.max_value<my_field_uintmax>() == std::numeric_limits<uintmax_t>::max());
    }
}

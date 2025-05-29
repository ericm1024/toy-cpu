#pragma once

#include "cpu_base.h"

static word_t constexpr k_page_size = 16384;

namespace iomap
{
    // IO
    static word_t constexpr k_console_base = k_page_size * 4;
    static word_t constexpr k_console_write = k_console_base;
    static word_t constexpr k_console_size = k_page_size;

    // Program memory
    static word_t constexpr k_rom_base = k_page_size * 5;
    static word_t constexpr k_rom_size = k_page_size;

    // Ram
    static word_t constexpr k_ram_base = k_page_size * 6;
    static word_t constexpr k_ram_size = k_page_size;
};

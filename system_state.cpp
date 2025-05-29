#include "system_state.h"
#include "instr.h"
#include "iomap.h"
#include "opcode.h"

system_state::system_state(std::span<word_t const> program)
    : rom{std::make_unique<uint8_t[]>(iomap::k_rom_size / sizeof(word_t))}
    , ram{std::make_unique<uint8_t[]>(iomap::k_ram_size / sizeof(word_t))}
{
    memset(rom.get(), 0, iomap::k_rom_size);
    memset(ram.get(), 0, iomap::k_ram_size);
    set_rom(program);
}

void system_state::set_rom(std::span<word_t const> program)
{
    set_rom(static_cast<void const *>(program.data()), program.size() * sizeof(word_t));
}

void system_state::set_rom(std::vector<instr> program)
{
    set_rom(static_cast<void const *>(program.data()), program.size() * sizeof(word_t));
}

void system_state::set_rom(void const * prog, size_t num_bytes)
{
    assert(num_bytes < iomap::k_rom_size);
    memcpy(rom.get(), prog, num_bytes);
}

void system_state::run()
{
    word_t instr_ptr = iomap::k_rom_base;

    while (true) {
        assert(instr_ptr - iomap::k_rom_base < iomap::k_rom_size);
        instr instr{raw_load(instr_ptr)};
        instr_ptr += k_word_size;
        printf("execute opcode %s\n", to_str(instr.get_opcode()));
        switch (instr.get_opcode()) {
        case opcode::set: {
            reg dest;
            word_t value;
            instr.decode_set(&dest, &value);
            execute_set(dest, value);
            break;
        }
        case opcode::store: {
            reg addr, src;
            word_t width_sel;
            instr.decode_store(&addr, &src, &width_sel);
            execute_store(addr, src, width_sel);
            break;
        }
        case opcode::load: {
            reg dest, addr;
            word_t width_sel;
            instr.decode_load(&dest, &addr, &width_sel);
            printf("execute load r%d = *r%d (0x%x)\n",
                   (unsigned)dest,
                   (unsigned)addr,
                   cpu.get(addr));
            execute_load(addr, dest, width_sel);
            break;
        }
        case opcode::add: {
            reg dest, op1, op2;
            instr.decode_add(&dest, &op1, &op2);
            execute_add(dest, op1, op2);
            break;
        }
        case opcode::halt:
            return;
        default:
            assert(false && "unknown opcode");
        }
    }
}

void system_state::execute_set(reg dest, word_t value)
{
    cpu.get(dest) = value;
}

void system_state::execute_store(reg addr_reg, reg value_reg, word_t width_sel)
{
    execute_load_store(false, addr_reg, value_reg, width_sel);
}

void system_state::execute_load(reg addr_reg, reg value_reg, word_t width_sel)
{
    execute_load_store(true, addr_reg, value_reg, width_sel);
}

void system_state::raw_store(word_t addr, word_t value)
{
    execute_load_store_impl(false, addr, &value, 2);
}

word_t system_state::raw_load(word_t addr)
{
    word_t tmp;
    execute_load_store_impl(true, addr, &tmp, 2);
    return tmp;
}

void system_state::execute_load_store(bool is_load, reg addr_reg, reg value_reg, word_t width_sel)
{
    word_t addr = cpu.get(addr_reg);
    word_t * value = &cpu.get(value_reg);
    execute_load_store_impl(is_load, addr, value, width_sel);
}

void system_state::execute_load_store_impl(bool is_load, word_t addr, word_t * value,
                                           word_t width_sel)
{
    word_t access_size = width_sel == 2 ? 4 : width_sel == 1 ? 2 : 1;
    assert(addr % access_size == 0);

    // console hardware special case
    if (addr >= iomap::k_console_base
        && addr <= iomap::k_console_base + iomap::k_console_size - access_size) {
        assert(access_size == 1 && is_load == false && addr == iomap::k_console_write);
        console.push_back(static_cast<uint8_t>(*value));
        return;
    }

    printf("execute_load_store_impl is_load=%d addr=0x%x access_size=%d\n",
           is_load,
           addr,
           access_size);

    uint8_t * mem_addr;
    if (addr >= iomap::k_ram_base && addr <= iomap::k_ram_base + iomap::k_ram_size - access_size) {
        mem_addr = ram.get() + (addr - iomap::k_ram_base);
    } else if (addr >= iomap::k_rom_base
               && addr <= iomap::k_rom_base + iomap::k_rom_size - access_size) {
        assert(is_load); // no writes to ROM
        mem_addr = rom.get() + (addr - iomap::k_rom_base);
    } else {
        assert(false);
    }

    // TODO: endian correctness
    if (is_load) {
        memcpy(value, mem_addr, access_size);
    } else {
        memcpy(mem_addr, value, access_size);
    }
}

void system_state::execute_add(reg dest, reg op1, reg op2)
{
    cpu.get(dest) = cpu.get(op1) + cpu.get(op2);
}

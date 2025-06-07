#include "system_state.h"

#include "instr.h"
#include "iomap.h"
#include "log.h"
#include "opcode.h"

#include <cstring>

static logger logger{__FILE__};

void cpu::add(reg dest, reg op1, reg op2)
{
    get(dest) = get(op1) + get(op2);
}

void cpu::sub(reg dest, reg op1, reg op2)
{
    get(dest) = get(op1) - get(op2);
}

void cpu::jump(cmp_flag flag, signed_word_t offset)
{
    if (is_taken(flag)) {
        instr_ptr += offset;
        // back one instruction so the increment at the end of execution takes us to the right
        // place.
        instr_ptr -= k_word_size;
    }
}

void cpu::ijump(cmp_flag flag, reg loc)
{
    if (is_taken(flag)) {
        instr_ptr = get(loc);
        // back one instruction so the increment at the end of execution takes us to the right
        // place.
        instr_ptr -= k_word_size;
    }
}

bool cpu::is_taken(cmp_flag flag) const
{
    switch (flag) {
    case instr::eq:
        return get_cmp_flag(cpu_cmp_flags::eq);
    case instr::ne:
        return !get_cmp_flag(cpu_cmp_flags::eq);
    case instr::gt:
        return get_cmp_flag(cpu_cmp_flags::gt);
    case instr::ge:
        return get_cmp_flag(cpu_cmp_flags::gt) || get_cmp_flag(cpu_cmp_flags::eq);
    case instr::lt:
        return get_cmp_flag(cpu_cmp_flags::lt);
    case instr::le:
        return get_cmp_flag(cpu_cmp_flags::lt) || get_cmp_flag(cpu_cmp_flags::eq);
    case instr::unc:
        return true;
    default:
        assert(false);
        return false;
    }
}

void cpu::call(signed_word_t offset)
{
    get(r15) = instr_ptr + k_word_size;
    instr_ptr += offset;
    instr_ptr -= k_word_size;
}

system_state::system_state(std::span<word_t const> program)
    : rom{std::make_unique<uint8_t[]>(iomap::k_rom_size)}
    , ram{std::make_unique<uint8_t[]>(iomap::k_ram_size)}
{
    memset(rom.get(), 0, iomap::k_rom_size);
    memset(ram.get(), 0, iomap::k_ram_size);
    if (program.size() > 0) {
        set_rom(program);
    }
}

void system_state::set_rom(std::span<word_t const> program)
{
    set_rom(static_cast<void const *>(program.data()), program.size() * sizeof(word_t));
}

void system_state::set_rom(std::span<uint8_t const> program)
{
    set_rom(static_cast<void const *>(program.data()), program.size());
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
    while (true) {
        assert(cpu.instr_ptr - iomap::k_rom_base < iomap::k_rom_size);
        instr instr{raw_load(cpu.instr_ptr)};
        logger.debug("[ip={:#x}] executing {}", cpu.instr_ptr, instr);
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
            word_t width;
            instr.decode_store(&addr, &src, &width);
            execute_store(addr, src, width);
            break;
        }
        case opcode::load: {
            reg dest, addr;
            word_t width;
            instr.decode_load(&dest, &addr, &width);
            execute_load(addr, dest, width);
            break;
        }
        case opcode::add: {
            reg dest, op1, op2;
            instr.decode_add(&dest, &op1, &op2);
            cpu.add(dest, op1, op2);
            break;
        }
        case opcode::sub: {
            reg dest, op1, op2;
            instr.decode_sub(&dest, &op1, &op2);
            cpu.sub(dest, op1, op2);
            break;
        }
        case opcode::halt:
            return;
        case opcode::compare: {
            reg op1, op2;
            instr.decode_compare(&op1, &op2);
            execute_compare(op1, op2);
            break;
        }
        case opcode::jump: {
            cmp_flag flag;
            signed_word_t offset;
            instr.decode_jump(&flag, &offset);
            cpu.jump(flag, offset);
            break;
        }
        case opcode::ijump: {
            cmp_flag flag;
            reg loc;
            instr.decode_ijump(&flag, &loc);
            cpu.ijump(flag, loc);
            break;
        }
        case opcode::call: {
            signed_word_t offset;
            instr.decode_call(&offset);
            cpu.call(offset);
            break;
        }
        default:
            assert(false && "unknown opcode");
        }
        cpu.instr_ptr += k_word_size;
    }
}

void system_state::execute_set(reg dest, word_t value)
{
    cpu.get(dest) = value;
}

void system_state::execute_store(reg addr_reg, reg value_reg, word_t width)
{
    execute_load_store(false, addr_reg, value_reg, width);
}

void system_state::execute_load(reg addr_reg, reg value_reg, word_t width)
{
    execute_load_store(true, addr_reg, value_reg, width);
}

void system_state::raw_store(word_t addr, word_t value)
{
    execute_load_store_impl(false, addr, &value, k_word_size);
}

word_t system_state::raw_load(word_t addr)
{
    word_t tmp;
    execute_load_store_impl(true, addr, &tmp, k_word_size);
    return tmp;
}

void system_state::execute_load_store(bool is_load, reg addr_reg, reg value_reg, word_t width)
{
    word_t addr = cpu.get(addr_reg);
    word_t * value = &cpu.get(value_reg);
    execute_load_store_impl(is_load, addr, value, width);
}

void system_state::execute_load_store_impl(bool is_load, word_t addr, word_t * value, word_t width)
{
    assert(addr % width == 0);

    // console hardware special case
    if (addr >= iomap::k_console_base
        && addr <= iomap::k_console_base + iomap::k_console_size - width) {
        assert(width == 1 && is_load == false && addr == iomap::k_console_write);
        console.push_back(static_cast<uint8_t>(*value));
        return;
    }

    uint8_t * mem_addr;
    if (addr >= iomap::k_ram_base && addr <= iomap::k_ram_base + iomap::k_ram_size - width) {
        mem_addr = ram.get() + (addr - iomap::k_ram_base);
    } else if (addr >= iomap::k_rom_base && addr <= iomap::k_rom_base + iomap::k_rom_size - width) {
        assert(is_load); // no writes to ROM
        mem_addr = rom.get() + (addr - iomap::k_rom_base);
    } else {
        assert(false);
    }

    // TODO: endian correctness
    if (is_load) {
        memcpy(value, mem_addr, width);
    } else {
        memcpy(mem_addr, value, width);
    }
}

void system_state::execute_compare(reg op1, reg op2)
{
    // clear invalid flag, flags are now valid
    cpu.cpu_cmp_flags = 0;
    if (cpu.get(op1) == cpu.get(op2)) {
        cpu.set_cmp_flag(cpu_cmp_flags::eq);
    } else if (cpu.get(op1) < cpu.get(op2)) {
        cpu.set_cmp_flag(cpu_cmp_flags::lt);
    } else {
        cpu.set_cmp_flag(cpu_cmp_flags::gt);
    }
}

#include "system_state.h"
#include "instr.h"
#include "iomap.h"
#include "log.h"
#include "opcode.h"

static logger logger;

system_state::system_state(std::span<word_t const> program)
    : rom{std::make_unique<uint8_t[]>(iomap::k_rom_size)}
    , ram{std::make_unique<uint8_t[]>(iomap::k_ram_size)}
{
    memset(rom.get(), 0, iomap::k_rom_size);
    memset(ram.get(), 0, iomap::k_ram_size);
    set_rom(program);
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
            execute_add(dest, op1, op2);
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
        case opcode::branch: {
            instr::cmp_flag flags;
            signed_word_t offset;
            instr.decode_branch(&flags, &offset);
            execute_branch(flags, offset);
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

    logger.debug("execute_load_store_impl is_load={} addr={:#x} width={}", is_load, addr, width);

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

void system_state::execute_add(reg dest, reg op1, reg op2)
{
    cpu.get(dest) = cpu.get(op1) + cpu.get(op2);
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

void system_state::execute_branch(instr::cmp_flag flags, signed_word_t offset)
{
    assert(!cpu.get_cmp_flag(cpu_cmp_flags::invalid));

    bool taken = false;
    switch (flags) {
    case instr::eq:
        taken = cpu.get_cmp_flag(cpu_cmp_flags::eq);
        break;
    case instr::ne:
        taken = !cpu.get_cmp_flag(cpu_cmp_flags::eq);
        break;
    case instr::gt:
        taken = cpu.get_cmp_flag(cpu_cmp_flags::gt);
        break;
    case instr::ge:
        taken = cpu.get_cmp_flag(cpu_cmp_flags::gt) || cpu.get_cmp_flag(cpu_cmp_flags::eq);
        break;
    case instr::lt:
        taken = cpu.get_cmp_flag(cpu_cmp_flags::lt);
        break;
    case instr::le:
        taken = cpu.get_cmp_flag(cpu_cmp_flags::lt) || cpu.get_cmp_flag(cpu_cmp_flags::eq);
        break;
    default:
        assert(false);
    }

    if (taken) {
        cpu.instr_ptr += offset;
        // back one instruction so the increment at the end of execution takes us to the right
        // place.
        cpu.instr_ptr -= k_word_size;
    }
}

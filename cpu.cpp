#include <cassert>
#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <vector>

// TODOs:
// * share load/store code
// * split to files
// * better test coverage
// * assembler
// * makefile
// * asan
// * mov instruction with immediate encoding

using word_t = uint32_t;

static word_t constexpr k_word_size = sizeof(word_t);

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

// 16 architectural registers
enum class reg : uint8_t
{
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
    num_registers,
};
using enum reg;

static size_t constexpr k_reg_bits = 4;
static word_t constexpr k_reg_mask = (word_t{1} << k_reg_bits) - 1;

static constexpr word_t raw(reg r)
{
    return static_cast<word_t>(r);
}

// instruction opcodes
enum class opcode : uint8_t
{
    set,
    store,
    add,
    halt,
    load,
};

static constexpr word_t raw(opcode op)
{
    return static_cast<word_t>(op);
}

static char const * const opcode_to_str[] = {
#define MAKE_ENTRY(op) [raw(opcode::op)] = #op,

    MAKE_ENTRY(set)
    MAKE_ENTRY(store)
    MAKE_ENTRY(add)
    MAKE_ENTRY(halt)
    MAKE_ENTRY(load)
};

static char const * to_str(opcode op)
{
    word_t index = raw(op);
    if (index >= std::size(opcode_to_str)) {
        return "unknown";
    }
    return opcode_to_str[index];
}

static size_t constexpr k_opcode_bits = sizeof(opcode) * 8;
static word_t constexpr k_opcode_mask = (word_t{1} << k_opcode_bits) - 1;

static size_t constexpr k_instr_bits = sizeof(word_t) * 8;

// Instructions are 32 bits
// First 8 bits are opcode, rest are opcode-dependent
struct instr
{
    explicit instr(word_t raw)
    {
        storage = raw;
    }

    instr(opcode op, word_t tail)
    {
        assert(tail < (1U << 24));
        storage = static_cast<uint8_t>(op) | (tail << k_opcode_bits);
    }

    opcode get_opcode() const
    {
        return static_cast<opcode>(storage & k_opcode_mask);
    }

    // value can be at most 20 bits
    static instr set(reg dest, word_t value)
    {
        assert(value < (1U << (k_instr_bits - k_opcode_bits - k_reg_bits)));
        return {opcode::set, raw(dest) | (value << k_reg_bits) };
    }

    void decode_set(reg * dest, word_t * value) const
    {
        assert(get_opcode() == opcode::set);
        word_t tmp = storage >> k_opcode_bits;
        *dest = static_cast<reg>(tmp & k_reg_mask);
        *value = tmp >> k_reg_bits;
    }

    static instr store(reg addr, reg src,
                       word_t width_sel /* 0, 1, 2 -> 1, 2, 4 bits*/)
    {
        assert(width_sel < 3);
        return {opcode::store, raw(addr) | raw(src) << (k_reg_bits) |
                width_sel << (2 * k_reg_bits) };
    }

    void decode_store(reg * addr, reg * src, word_t * width_sel) const
    {
        assert(get_opcode() == opcode::store);
        word_t tmp = storage >> k_opcode_bits;
        *addr = static_cast<reg>(tmp & k_reg_mask);
        tmp >>= k_reg_bits;
        *src = static_cast<reg>(tmp & k_reg_mask);
        tmp >>= k_reg_bits;
        *width_sel = tmp & 0x3;
        assert(*width_sel < 3);
    }

    static instr store1(reg addr, reg src)
    {
        return store(addr, src, 0);
    }

    static instr store2(reg addr, reg src)
    {
        return store(addr, src, 1);
    }

    static instr store4(reg addr, reg src)
    {
        return store(addr, src, 2);
    }

    static instr add(reg dest, reg op1, reg op2)
    {
        return {opcode::add, raw(dest) | (raw(op1) << k_reg_bits) | (raw(op2) << (2 * k_reg_bits))};
    }

    void decode_add(reg * dest, reg * op1, reg * op2)
    {
        word_t tmp = storage >> k_opcode_bits;
        *dest = static_cast<reg>(tmp & k_reg_mask);
        tmp >>= k_reg_bits;
        *op1 = static_cast<reg>(tmp & k_reg_mask);
        tmp >>= k_reg_bits;
        *op2 = static_cast<reg>(tmp & k_reg_mask);
    }

    static instr halt()
    {
        return {opcode::halt, 0};
    }

    static instr load(reg addr, reg src,
                       word_t width_sel /* 0, 1, 2 -> 1, 2, 4 bits*/)
    {
        assert(width_sel < 3);
        return {opcode::load, raw(addr) | raw(src) << (k_reg_bits) |
                width_sel << (2 * k_reg_bits) };
    }

    void decode_load(reg * dest, reg * src, word_t * width_sel) const
    {
        assert(get_opcode() == opcode::load);
        word_t tmp = storage >> k_opcode_bits;
        *dest = static_cast<reg>(tmp & k_reg_mask);
        tmp >>= k_reg_bits;
        *src = static_cast<reg>(tmp & k_reg_mask);
        tmp >>= k_reg_bits;
        *width_sel = tmp & 0x3;
        assert(*width_sel < 3);
    }

    static instr load1(reg dest, reg src)
    {
        return load(dest, src, 0);
    }

    static instr load2(reg dest, reg src)
    {
        return load(dest, src, 1);
    }

    static instr load4(reg dest, reg src)
    {
        return load(dest, src, 2);
    }

    word_t storage;
};

struct cpu
{
    word_t & get(reg reg)
    {
        word_t index = raw(reg);
        assert(index < std::size(registers));
        return registers[index];
    }

    word_t registers[raw(reg::num_registers)]{};
};

struct system_state
{
    system_state(std::span<word_t const> program)
        : rom{std::make_unique<uint32_t[]>(iomap::k_rom_size/sizeof(word_t))}
        , ram{std::make_unique<uint32_t[]>(iomap::k_ram_size/sizeof(word_t))}
    {
        memset(rom.get(), 0, iomap::k_rom_size);
        memset(ram.get(), 0, iomap::k_ram_size);
        assert(program.size() < iomap::k_rom_size/sizeof(word_t));
        memcpy(rom.get(), program.data(), program.size() * sizeof(word_t));
    }

    void run();

    void execute_set(reg dest, word_t value);
    void execute_store(reg addr, reg src, word_t width_sel);
    void execute_add(reg dest, reg op1, reg op2);
    void execute_load(reg dest, reg src, word_t width_sel);

    std::vector<uint8_t> console;
    std::unique_ptr<word_t[]> rom;
    std::unique_ptr<word_t[]> ram;
    cpu cpu;
};

void system_state::run()
{
    word_t * instr_ptr = rom.get();

    while (true) {
        assert(instr_ptr < rom.get() + iomap::k_rom_size);
        instr instr{*instr_ptr++};
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
        case opcode::add: {
            reg dest, op1, op2;
            instr.decode_add(&dest, &op1, &op2);
            execute_add(dest, op1, op2);
            break;
        }
        case opcode::halt:
            return;
        case opcode::load: {
            reg dest, src;
            word_t width_sel;
            instr.decode_load(&dest, &src, &width_sel);
            execute_load(dest, src, width_sel);
            break;
        }
        default:
            assert(false && "unknown opcode");
        }
    }
}

void system_state::execute_set(reg dest, word_t value)
{
    cpu.get(dest) = value;
}

void system_state::execute_store(reg addr, reg src, word_t width_sel)
{
    word_t dest_addr = cpu.get(addr);
    if (width_sel == 0) {
        if (dest_addr == iomap::k_console_write) {
            console.push_back(static_cast<uint8_t>(cpu.get(src)));
        } else {
            assert(false && "unhandled addr");
        }
    } else if (width_sel == 2) {
        assert((dest_addr % k_word_size) == 0);
        if (dest_addr >= iomap::k_ram_base && dest_addr <= iomap::k_ram_base + iomap::k_ram_size - k_word_size) {
            *(ram.get() + (dest_addr - iomap::k_ram_base)/k_word_size) = cpu.get(src);
        }
    } else {
        assert(false && "unhandled width_sel");
    }
}

void system_state::execute_add(reg dest, reg op1, reg op2)
{
    cpu.get(dest) = cpu.get(op1) + cpu.get(op2);
}

void system_state::execute_load(reg dest, reg src, word_t width_sel)
{
    if (width_sel == 2) {
        word_t src_addr = cpu.get(src);
        assert(((src_addr % k_word_size) == 0) && "unaligned load");

        word_t val;
        if (src_addr >= iomap::k_ram_base && src_addr <= iomap::k_ram_base + iomap::k_ram_size - k_word_size) {
            val = *(ram.get() + (src_addr - iomap::k_ram_base)/k_word_size);
        } else if (src_addr >= iomap::k_rom_base && src_addr <= iomap::k_rom_base + iomap::k_rom_size - k_word_size) {
            val = *(rom.get() + (src_addr - iomap::k_rom_base)/k_word_size);
        } else {
            assert(false && "unhandled load addr");
        }
        printf("load r%d = %d\n", raw(dest), val);
        cpu.get(dest) = val;
    } else {
        assert(false && "unhandled width_sel");
    }
}

static void test_enc_dec_set()
{
    instr ii = instr::set(r1, 42);
    reg dest;
    word_t value;
    ii.decode_set(&dest, &value);
    assert(dest == r1);
    assert(value == 42);
}

static void test_enc_dec_add()
{
    instr ii = instr::add(r1, r2, r3);
    reg dest, op1, op2;
    ii.decode_add(&dest, &op1, &op2);
    assert(dest == r1);
    assert(op1 == r2);
    assert(op2 == r3);
}

// hello world program
static std::vector<word_t> make_hello_world_rom()
{
    std::vector<word_t> rom;
    char const * str = "Hello world!";
    rom.push_back(instr::set(r0, iomap::k_console_write).storage);
    for (size_t i = 0; i < strlen(str); ++i) {
        rom.push_back(instr::set(r1, str[i]).storage);
        rom.push_back(instr::store1(r0, r1).storage);
    }
    rom.push_back(instr::halt().storage);
    return rom;
}

static void test_hello_world_rom()
{
    system_state system{make_hello_world_rom()};
    system.run();
    system.console.push_back('\0');
    assert(strcmp((char const *)system.console.data(), "Hello world!") == 0);
}

// reads 42 and 43 from rom, adds them writes the result to ram
static std::vector<word_t> make_add_numbers_rom()
{
    std::vector<word_t> rom;
    rom.resize(128);

    word_t const k_nums_offset = 64 * k_word_size;
    rom[k_nums_offset/k_word_size] = 42;
    rom[k_nums_offset/k_word_size + 1] = 43;

    // r0 <- k_rom_base + k_nums_offset
    size_t i_instr = 0;
    rom[i_instr++] = instr::set(r0, iomap::k_rom_base + k_nums_offset).storage;

    // r1 <- *r0     ;; r1 holds 42
    rom[i_instr++] = instr::load4(r1, r0).storage;

    // r2 = 4
    rom[i_instr++] = instr::set(r2, 4).storage;

    // r0 = r0 + r2
    rom[i_instr++] = instr::add(r0, r0, r2).storage;

    // r3 <- *r0       ;; r3 holds 43
    rom[i_instr++] = instr::load4(r3, r0).storage;

    // r1 <- r1 + r3   ;; r1 = 42 + 43
    rom[i_instr++] = instr::add(r1, r1, r3).storage;

    // r0 = k_ram_base
    rom[i_instr++] = instr::set(r0, iomap::k_ram_base).storage;

    // *r0 = r1
    rom[i_instr++] = instr::store4(r0, r1).storage;

    rom[i_instr++] = instr::halt().storage;
    return rom;
}

static void test_add_numbers_rom()
{
    system_state system{make_add_numbers_rom()};
    system.run();
    assert(*system.ram.get() == 42 + 43);
}

static void run_tests()
{
    test_enc_dec_set();
    test_enc_dec_add();
    test_hello_world_rom();
    test_add_numbers_rom();
}

enum class command
{
    run_tests,
    run_prog
};

int main(int argc, char ** argv)
{
    command cmd;
    if (argc > 2) {
        fprintf(stderr, "too many arguments\n");
        return 1;
    } else if (argc == 2) {
        if (argv[1] == std::string{"tests"}) {
            cmd = command::run_tests;
        } else if (argv[1] == std::string{"prog"}) {
            cmd = command::run_prog;
        } else {
            fprintf(stderr, "invalid command %s\n", argv[1]);
            return 1;
        }
    } else {
        cmd = command::run_prog;
    }

    if (cmd == command::run_prog) {
        system_state state{make_hello_world_rom()};
        state.run();
        state.console.push_back('\0');
        printf("program prints: %s\n", reinterpret_cast<char const *>(state.console.data()));
    } else if (cmd == command::run_tests) {
        run_tests();
        return 0;
    }
}

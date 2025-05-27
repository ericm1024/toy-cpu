#include <cassert>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

using word_t = uint32_t;

static word_t constexpr k_page_size = 16384;

namespace iomap
{
    // IO
    static word_t constexpr k_console_base = k_page_size * 4;
    static word_t constexpr k_console_write = k_console_base;
    static word_t constexpr k_console_size = k_page_size;

    // Program memory
    static word_t constexpr k_rom_base = 2 << 20;
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
};

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

    opcode get_opcode() const
    {
        return static_cast<opcode>(storage & k_opcode_mask);
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

// hello world program
static std::vector<uint32_t> make_hello_world_rom()
{
    std::vector<uint32_t> rom;
    char const * str = "Hello world!";
    rom.push_back(instr::set(r0, iomap::k_console_write).storage);
    for (size_t i = 0; i < strlen(str); ++i) {
        rom.push_back(instr::set(r1, str[i]).storage);
        rom.push_back(instr::store1(r0, r1).storage);
    }
    return rom;
}

static std::vector<uint8_t> run_program(std::span<uint32_t const> program)
{
    cpu cpu;
    std::vector<uint8_t> console;
    for (word_t word : program) {
        instr instr{word};
        switch (instr.get_opcode()) {
        case opcode::set: {
            reg dest;
            word_t value;
            instr.decode_set(&dest, &value);
            cpu.get(dest) = value;
            break;
        }
        case opcode::store: {
            reg addr;
            reg src;
            word_t width_sel;
            instr.decode_store(&addr, &src, &width_sel);
            if (width_sel == 0) {
                if (cpu.get(addr) == iomap::k_console_write) {
                    console.push_back(static_cast<uint8_t>(cpu.get(src)));
                } else {
                    assert(false && "unhandled addr");
                }
            } else {
                assert(false && "unhandled width_sel");
            }
            break;
        }

        }
    }
    return console;
}

static void test_set()
{
    instr ii = instr::set(r1, 42);
    reg dest;
    word_t value;
    ii.decode_set(&dest, &value);
    assert(dest == r1);
    assert(value == 42);
}

static void run_tests()
{
    test_set();
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
        std::vector<uint8_t> console = run_program(make_hello_world_rom());
        console.push_back('\0');
        printf("program prints: %s\n", reinterpret_cast<char const *>(console.data()));
    } else if (cmd == command::run_tests) {
        run_tests();
        return 0;
    }
}

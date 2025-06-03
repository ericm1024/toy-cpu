#include "assembler.h"
#include "instr.h"
#include "log.h"
#include "opcode.h"
#include "reg.h"

#include <cassert>
#include <charconv>
#include <span>

static logger logger;

struct instr_assembler
{
    instr_assembler(std::span<std::string_view> tokens, std::vector<uint8_t> * rom)
        : tokens_{tokens}
        , rom_{rom}
    { }

    void assemble();

private:
    void push_instr(instr ii);

    void assemble_set();

    void assemble_load_store(word_t width, bool is_load);

    template <word_t width, bool is_load>
    void assemble_load_store()
    {
        assemble_load_store(width, is_load);
    }

    void assemble_add();

    void assemble_halt();

    using assemble_fn = void (instr_assembler::*)();

    static inline std::pair<char const *, assemble_fn> dispatch_table[]{
        {"set", &instr_assembler::assemble_set},
        {"store.1", &instr_assembler::assemble_load_store<1, false>},
        {"store.2", &instr_assembler::assemble_load_store<2, false>},
        {"store.4", &instr_assembler::assemble_load_store<4, false>},
        {"store", &instr_assembler::assemble_load_store<4, false>},
        {"load.1", &instr_assembler::assemble_load_store<1, true>},
        {"load.2", &instr_assembler::assemble_load_store<2, true>},
        {"load.4", &instr_assembler::assemble_load_store<4, true>},
        {"load", &instr_assembler::assemble_load_store<4, true>},
        {"add", &instr_assembler::assemble_add},
        {"halt", &instr_assembler::assemble_halt},
    };

    std::span<std::string_view> tokens_;
    std::vector<uint8_t> * const rom_;
};

void instr_assembler::assemble()
{
    for (auto & [str, fn] : dispatch_table) {
        if (tokens_[0] == str) {
            tokens_ = tokens_.subspan(1);
            (this->*fn)();
            return;
        }
    }
    assert(false);
}

void instr_assembler::push_instr(instr ii)
{
    auto it = rom_->insert(rom_->end(), sizeof(word_t), 0);
    logger.debug("rom->size() = {}", rom_->size());
    memcpy(&*it, &ii.storage, sizeof(word_t));
}

void instr_assembler::assemble_set()
{
    assert(tokens_.size() == 2);
    std::optional<reg> dest_reg = from_str<reg>(tokens_[0]);
    assert(dest_reg.has_value());

    word_t value;
    std::from_chars_result result = std::from_chars(tokens_[1].begin(), tokens_[1].end(), value);
    assert(result.ec == std::errc{});
    assert(result.ptr == tokens_[1].end());
    assert(value < (1 << 20));

    push_instr(instr::set(*dest_reg, value));
}

void instr_assembler::assemble_load_store(word_t width, bool is_load)
{
    assert(tokens_.size() == 2);

    std::optional<reg> lhs_reg = from_str<reg>(tokens_[0]);
    assert(lhs_reg.has_value());

    std::optional<reg> rhs_reg = from_str<reg>(tokens_[1]);
    assert(rhs_reg.has_value());

    auto ctor = is_load ? &instr::load : &instr::store;
    push_instr(ctor(*lhs_reg, *rhs_reg, width));
}

void instr_assembler::assemble_add()
{
    assert(tokens_.size() == 3);

    std::optional<reg> dst_reg = from_str<reg>(tokens_[0]);
    assert(dst_reg.has_value());

    std::optional<reg> op1_reg = from_str<reg>(tokens_[1]);
    assert(op1_reg.has_value());

    std::optional<reg> op2_reg = from_str<reg>(tokens_[2]);
    assert(op2_reg.has_value());

    push_instr(instr::add(*dst_reg, *op1_reg, *op2_reg));
}

void instr_assembler::assemble_halt()
{
    assert(tokens_.size() == 0);

    push_instr(instr::halt());
}

static std::vector<std::string_view> tokenize_line(std::string_view line)
{
    size_t start = 0;
    std::vector<std::string_view> tokens;
    while (true) {
        start = line.find_first_not_of(' ', start);
        if (start == std::string_view::npos) {
            break;
        }
        size_t end = line.find(' ', start);
        if (end == std::string_view::npos) {
            end = line.size();
        }
        std::string_view token = line.substr(start, end - start);
        if (token.size() == 0) {
            break;
        }
        start = end;

        logger.debug("parse token {}, size {}", token, token.size());

        tokens.push_back(token);
    }
    assert(tokens.size() != 0);
    return tokens;
}

std::vector<uint8_t> assemble(std::string_view program)
{
    std::vector<uint8_t> rom;

    size_t start = 0;
    while (start < program.size()) {
        size_t end = program.find('\n', start);
        if (end == std::string_view::npos) {
            end = program.size();
        }
        logger.debug("start={}, end={}", start, end);

        std::string_view line = program.substr(start, end - start);
        if (line.size() == 0) {
            break;
        }
        start = end + 1;

        logger.debug("parse line {}", line);

        std::vector<std::string_view> tokens = tokenize_line(line);
        instr_assembler assembler{tokens, &rom};
        assembler.assemble();
    }
    return rom;
}

std::string disassemble(std::span<uint8_t const> rom)
{
    assert(rom.size() % k_word_size == 0);

    std::string ret;
    for (size_t offset = 0; offset < rom.size(); offset += k_word_size) {
        if (offset != 0) {
            ret += "\n";
        }

        word_t raw_instr;
        memcpy(&raw_instr, rom.data() + offset, sizeof(raw_instr));

        instr ii{raw_instr};
        ret += std::format("{}", ii);
    }
    return ret;
}

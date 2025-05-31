#include "assembler.h"
#include "instr.h"
#include "opcode.h"
#include "reg.h"

#include <cassert>
#include <charconv>
#include <span>

struct instr_assembler
{
    instr_assembler(std::span<std::string_view> tokens, std::vector<uint8_t> * rom)
        : tokens_{tokens}
        , rom_{rom}
    {
    }

    void assemble();

private:
    void push_instr(instr ii);

    void assemble_set();

    using assemble_fn = void (instr_assembler::*)();

    static inline std::pair<char const *, assemble_fn> dispatch_table[]{
        {"set", &instr_assembler::assemble_set},
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
    printf("rom->size() = %zu\n", rom_->size());
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

        printf("parse token %.*s, size %zu\n",
               static_cast<int>(token.size()),
               token.data(),
               token.size());

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
        printf("start=%zu, end=%zu\n", start, end);

        std::string_view line = program.substr(start, end - start);
        if (line.size() == 0) {
            break;
        }
        start = end + 1;

        printf("parse line %.*s\n", static_cast<int>(line.size()), line.data());

        std::vector<std::string_view> tokens = tokenize_line(line);
        instr_assembler assembler{tokens, &rom};
        assembler.assemble();
    }
    return rom;
}

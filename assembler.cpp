#include "assembler.h"

#include "cpu_base.h"
#include "instr.h"
#include "log.h"
#include "opcode.h"
#include "reg.h"

#include <algorithm>
#include <cassert>
#include <charconv>
#include <cstring>
#include <ctype.h>
#include <format>
#include <optional>
#include <span>
#include <system_error>
#include <unordered_map>
#include <utility>

static logger logger{__FILE__};

struct instr_assembler
{
    instr_assembler(std::span<std::string_view> tokens, word_t instr_offset,
                    std::unordered_map<std::string_view, word_t> const & labels,
                    std::vector<uint8_t> * rom)
        : tokens_{tokens}
        , instr_offset_{instr_offset}
        , labels_{labels}
        , rom_{rom}
    { }

    void assemble();

private:
    template <typename word_type>
    word_type parse_word(std::string_view token)
    {
        word_type value;
        std::from_chars_result result = std::from_chars(token.begin(), token.end(), value);
        assert(result.ec == std::errc{});
        assert(result.ptr == token.end());
        return value;
    }

    void push_instr(instr ii);

    void assemble_set();

    void assemble_load_store(word_t width, bool is_load);

    template <word_t width, bool is_load>
    void assemble_load_store()
    {
        assemble_load_store(width, is_load);
    }

    void assemble_add();

    void assemble_sub();

    void assemble_halt();

    void assemble_compare();

    void assemble_jump(cmp_flag flag);

    template <cmp_flag flag>
    void assemble_jump()
    {
        assemble_jump(flag);
    }

    void assemble_ijump(cmp_flag flag);

    template <cmp_flag flag>
    void assemble_ijump()
    {
        assemble_ijump(flag);
    }

    void assemble_call();

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
        {"sub", &instr_assembler::assemble_sub},
        {"halt", &instr_assembler::assemble_halt},
        {"compare", &instr_assembler::assemble_compare},
        {"jump.eq", &instr_assembler::assemble_jump<instr::eq>},
        {"jump.ne", &instr_assembler::assemble_jump<instr::ne>},
        {"jump.gt", &instr_assembler::assemble_jump<instr::gt>},
        {"jump.ge", &instr_assembler::assemble_jump<instr::ge>},
        {"jump.lt", &instr_assembler::assemble_jump<instr::lt>},
        {"jump.le", &instr_assembler::assemble_jump<instr::le>},
        {"jump", &instr_assembler::assemble_jump<instr::unc>},
        {"jump.unc", &instr_assembler::assemble_jump<instr::unc>},
        {"ijump.eq", &instr_assembler::assemble_ijump<instr::eq>},
        {"ijump.ne", &instr_assembler::assemble_ijump<instr::ne>},
        {"ijump.gt", &instr_assembler::assemble_ijump<instr::gt>},
        {"ijump.ge", &instr_assembler::assemble_ijump<instr::ge>},
        {"ijump.lt", &instr_assembler::assemble_ijump<instr::lt>},
        {"ijump.le", &instr_assembler::assemble_ijump<instr::le>},
        {"ijump", &instr_assembler::assemble_ijump<instr::unc>},
        {"ijump.unc", &instr_assembler::assemble_ijump<instr::unc>},
        {"call", &instr_assembler::assemble_call},
    };

    std::span<std::string_view> tokens_;
    word_t const instr_offset_;
    std::unordered_map<std::string_view, word_t> const & labels_;
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
    logger.debug("rom->size() = {}, emit instr {}", rom_->size(), ii);
    memcpy(&*it, &ii.storage, sizeof(word_t));
}

void instr_assembler::assemble_set()
{
    assert(tokens_.size() == 2);
    std::optional<reg> dest_reg = from_str<reg>(tokens_[0]);
    assert(dest_reg.has_value());

    word_t value = parse_word<word_t>(tokens_[1]);
    assert(value < instr::k_max_set_value);

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

void instr_assembler::assemble_sub()
{
    assert(tokens_.size() == 3);

    std::optional<reg> dst_reg = from_str<reg>(tokens_[0]);
    assert(dst_reg.has_value());

    std::optional<reg> op1_reg = from_str<reg>(tokens_[1]);
    assert(op1_reg.has_value());

    std::optional<reg> op2_reg = from_str<reg>(tokens_[2]);
    assert(op2_reg.has_value());

    push_instr(instr::sub(*dst_reg, *op1_reg, *op2_reg));
}

void instr_assembler::assemble_halt()
{
    assert(tokens_.size() == 0);

    push_instr(instr::halt());
}

void instr_assembler::assemble_compare()
{
    assert(tokens_.size() == 2);

    std::optional<reg> op1_reg = from_str<reg>(tokens_[0]);
    assert(op1_reg.has_value());

    std::optional<reg> op2_reg = from_str<reg>(tokens_[1]);
    assert(op2_reg.has_value());

    push_instr(instr::compare(*op1_reg, *op2_reg));
}

void instr_assembler::assemble_jump(cmp_flag flag)
{
    assert(tokens_.size() == 1);

    auto it = labels_.find(tokens_[0]);
    signed_word_t offset;
    if (it != labels_.end()) {
        offset = it->second - instr_offset_;
    } else {
        offset = parse_word<signed_word_t>(tokens_[0]);
    }
    assert(instr::k_jump_min_offset <= offset && offset <= instr::k_jump_max_offset);
    push_instr(instr::jump(flag, offset));
}

void instr_assembler::assemble_ijump(cmp_flag flag)
{
    assert(tokens_.size() == 1);

    std::optional<reg> loc = from_str<reg>(tokens_[0]);
    assert(loc.has_value());

    push_instr(instr::ijump(flag, *loc));
}

void instr_assembler::assemble_call()
{
    assert(tokens_.size() == 1);

    auto it = labels_.find(tokens_[0]);
    signed_word_t offset;
    if (it != labels_.end()) {
        offset = it->second - instr_offset_;
    } else {
        offset = parse_word<signed_word_t>(tokens_[0]);
    }

    assert(instr::k_call_min_offset <= offset && offset <= instr::k_call_max_offset);
    push_instr(instr::call(offset));
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
    return tokens;
}

static bool is_comment(std::string_view token)
{
    return token[0] == '#';
}

static void strip_comments(std::vector<std::string_view> * tokens)
{
    tokens->erase(std::find_if(tokens->begin(), tokens->end(), is_comment), tokens->end());
}

static bool is_label(std::span<std::string_view const> tokens, std::string_view * label)
{
    if (tokens.size() == 1 && tokens[0].back() == ':') {
        *label = tokens[0].substr(0, tokens[0].size() - 1);
        assert(!from_str<reg>(*label) && !from_str<opcode>(*label) && label->size() > 0
               && isalpha(label->front()));
        return true;
    }
    return false;
}

std::vector<uint8_t> assemble(std::string_view program)
{
    std::vector<uint8_t> rom;

    // TODO: eventually some mnemonics may emit multiple instructions, so this way of doing things
    // is fairly brittle. Instead of computing label offsets up front, then emitting instructions,
    // I think we want to emit instructions first, then fill in ijump offsets at the very end
    // once we've decided where everything is going to live.
    std::unordered_map<std::string_view, word_t> labels;
    std::vector<std::vector<std::string_view>> lines;
    size_t start = 0;
    word_t word_offset = 0;
    while (start < program.size()) {
        size_t end = program.find('\n', start);
        if (end == std::string_view::npos) {
            end = program.size();
            if (end == start) {
                break;
            }
        }
        logger.debug("start={}, end={}", start, end);

        std::string_view line = program.substr(start, end - start);
        start = end + 1;

        logger.debug("parse line {}", line);

        std::vector<std::string_view> tokens = tokenize_line(line);
        strip_comments(&tokens);
        if (tokens.size() == 0) {
            continue;
        }

        std::string_view label;
        if (is_label(tokens, &label)) {
            labels.emplace(label, word_offset);
            continue;
        }

        lines.push_back(tokens);

        word_offset += k_word_size;
    }

    word_offset = 0;
    for (auto & line : lines) {
        instr_assembler assembler{line, word_offset, labels, &rom};
        assembler.assemble();
        word_offset += k_word_size;
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

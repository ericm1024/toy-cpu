#pragma once

#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

std::vector<uint8_t> assemble(std::string_view prog);

std::string disassemble(std::span<uint8_t const> rom);

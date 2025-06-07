#include "reg.h"

#include <iterator>
#include <string>

#define ENUM_DEF_FILE_NAME "reg_def.h"
#include "enum_def.h"

std::initializer_list<reg> const k_all_registers{
#define X(x) x,
#include "reg_def.h"
#undef X
};

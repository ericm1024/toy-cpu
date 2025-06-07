
CXX=clang++
WARNINGS=-Werror -Wall -Wextra -Wswitch-enum -Wno-c99-designator
CXXFLAGS=$(WARNINGS) -std=c++23 -g -stdlib=libc++
COV_FLAGS=-fprofile-instr-generate -fcoverage-mapping
DBG_FLAGS=-fsanitize=address -fsanitize=undefined

SRCS := $(wildcard *.cpp)
COV_DIR=$(CURDIR)/cov
BIN_DIR=$(CURDIR)/bin

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(BIN_DIR)/cpu-dbg: $(BIN_DIR) $(SRCS) *.h
	$(CXX) $(CXXFLAGS) $(DBG_FLAGS) -o $@ $(SRCS)

$(BIN_DIR)/cpu-cov: $(BIN_DIR) $(SRCS) *.h
	$(CXX) $(CXXFLAGS) $(COV_FLAGS) -o $@ $(SRCS)

.PHONY: tests
tests: $(BIN_DIR)/cpu-dbg
	$(BIN_DIR)/cpu-dbg

.PHONY: coverage
coverage: $(BIN_DIR)/cpu-cov
	LLVM_PROFILE_FILE=$(COV_DIR)/cpu.profraw $(BIN_DIR)/cpu-cov
	llvm-profdata merge $(COV_DIR)/cpu.profraw -o $(COV_DIR)/cpu.profdata
	llvm-cov show -instr-profile $(COV_DIR)/cpu.profdata -format html -object $(BIN_DIR)/cpu-cov -output-dir $(COV_DIR) -show-branches count

.PHONY: clean
clean:
	rm -rf $(BIN_DIR) $(COV_DIR)

.PHONY: format
format:
	git ls-files | grep ".*\.\(cpp\|h\)$$" | grep -v 'enum_def.h' | xargs clang-format --style=file -i

.PHONY: iwyu-impl
iwyu-impl:
	echo $(SRCS) | xargs -n1 include-what-you-use $(CXXFLAGS) 2>&1 | fix_include

.PHONY: iwyu
iwyu: iwyu-impl format

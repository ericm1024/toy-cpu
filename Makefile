
CXX=clang++
CXXFLAGS=-Wall -Wextra -pedantic -std=c++20 -g -fsanitize=address -fsanitize=undefined -fprofile-instr-generate -fcoverage-mapping

SRCS := $(wildcard *.cpp)
COV_DIR=$(CURDIR)/cov

cpu: $(SRCS) *.h
	$(CXX) $(CXXFLAGS) -o $@ $(SRCS)

.PHONY: tests
tests: cpu
	LLVM_PROFILE_FILE=$(COV_DIR)/cpu.profraw ./cpu
	llvm-profdata merge -sparse $(COV_DIR)/cpu.profraw -o $(COV_DIR)/cpu.profdata
	llvm-cov show -instr-profile $(COV_DIR)/cpu.profdata -format html -object cpu -output-dir $(COV_DIR) -show-branches count

.PHONY: clean
clean:
	rm -rf $(CURDIR)/cpu
	rm -rf $(CURDIR)/cov

.PHONY: format
format:
	git ls-files | grep ".*\.\(cpp\|h\)$$" | xargs clang-format --style=file -i

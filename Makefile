
CXX=clang++
CXXFLAGS=-Wall -Wextra -pedantic -std=c++20 -g -fsanitize=address -fsanitize=undefined

SRCS := $(wildcard *.cpp)

cpu: $(SRCS) *.h
	$(CXX) $(CXXFLAGS) -o $@ $(SRCS)

.PHONY: tests
tests: cpu
	./cpu

.PHONY: clean
clean:
	rm -rf cpu

.PHONY: format
format:
	git ls-files | grep ".*\.\(cpp\|h\)$$" | xargs clang-format --style=file -i

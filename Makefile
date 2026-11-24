CXX ?= g++
CPPFLAGS ?= -Isrc
WARNINGS := -Wall -Wextra -Wpedantic
COMMON_FLAGS := -std=c++20 $(WARNINGS)
RELEASE_FLAGS ?= -O3 -DNDEBUG
DEBUG_FLAGS ?= -O1 -g -fsanitize=address,undefined
LDFLAGS ?=

ENGINE_SOURCES := src/bitboard.cpp src/position.cpp src/movegen.cpp src/eval.cpp src/search.cpp src/bench.cpp src/uci.cpp
SOURCES := $(ENGINE_SOURCES) src/main.cpp
BUILD_DIR := build
RELEASE_BIN := bubu
DEBUG_BIN := $(BUILD_DIR)/bubu-debug

.PHONY: all release debug test bench clean

all: release

release: $(RELEASE_BIN)

$(RELEASE_BIN): $(SOURCES) tests/perft.epd | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(COMMON_FLAGS) $(RELEASE_FLAGS) $(SOURCES) $(LDFLAGS) -o $@

debug: $(DEBUG_BIN)

$(DEBUG_BIN): $(SOURCES) tests/perft.epd | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(COMMON_FLAGS) $(DEBUG_FLAGS) $(SOURCES) $(LDFLAGS) -o $@

$(BUILD_DIR):
	mkdir -p $@

test: release
	./$(RELEASE_BIN) test
	python3 tools/test_uci.py ./$(RELEASE_BIN)
	python3 tools/test_bench.py ./$(RELEASE_BIN)

bench: release
	./$(RELEASE_BIN) bench

clean:
	rm -rf $(BUILD_DIR) $(RELEASE_BIN)

CXX ?= g++
CPPFLAGS ?= -Isrc
WARNINGS := -Wall -Wextra -Wpedantic
COMMON_FLAGS := -std=c++20 $(WARNINGS)
RELEASE_FLAGS ?= -O3 -DNDEBUG
DEBUG_FLAGS ?= -O1 -g -fsanitize=address,undefined
LDFLAGS ?=

ENGINE_SOURCES := src/bitboard.cpp src/position.cpp src/movegen.cpp src/eval.cpp src/search.cpp src/bench.cpp src/uci.cpp
SOURCES := $(ENGINE_SOURCES) src/main.cpp
HEADERS := $(wildcard src/*.h)
BUILD_DIR := build
RELEASE_BIN := bubu
DEBUG_BIN := $(BUILD_DIR)/bubu-debug
NATIVE_BIN := $(BUILD_DIR)/bubu-native
V2_BIN := $(BUILD_DIR)/bubu-v2
V3_BIN := $(BUILD_DIR)/bubu-v3
NATIVE_FLAGS := -O3 -DNDEBUG -march=native -flto
V2_FLAGS := -O3 -DNDEBUG -march=x86-64-v2
V3_FLAGS := -O3 -DNDEBUG -march=x86-64-v3

.PHONY: all release release-native release-v2 release-v3 debug test bench clean

all: release

release: $(RELEASE_BIN)

# These architecture-specific builds are for local machine comparisons and
# deployments that explicitly match the selected x86-64 ISA level.
release-native: $(NATIVE_BIN)

release-v2: $(V2_BIN)

release-v3: $(V3_BIN)

$(RELEASE_BIN): $(SOURCES) $(HEADERS) tests/perft.epd | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(COMMON_FLAGS) $(RELEASE_FLAGS) $(SOURCES) $(LDFLAGS) -o $@

debug: $(DEBUG_BIN)

$(DEBUG_BIN): $(SOURCES) $(HEADERS) tests/perft.epd | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(COMMON_FLAGS) $(DEBUG_FLAGS) $(SOURCES) $(LDFLAGS) -o $@

$(NATIVE_BIN): $(SOURCES) $(HEADERS) tests/perft.epd | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(COMMON_FLAGS) $(NATIVE_FLAGS) $(SOURCES) $(LDFLAGS) -o $@

$(V2_BIN): $(SOURCES) $(HEADERS) tests/perft.epd | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(COMMON_FLAGS) $(V2_FLAGS) $(SOURCES) $(LDFLAGS) -o $@

$(V3_BIN): $(SOURCES) $(HEADERS) tests/perft.epd | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(COMMON_FLAGS) $(V3_FLAGS) $(SOURCES) $(LDFLAGS) -o $@

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

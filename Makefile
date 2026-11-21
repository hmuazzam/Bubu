CXX ?= g++
CPPFLAGS ?= -Isrc
WARNINGS := -Wall -Wextra -Wpedantic
COMMON_FLAGS := -std=c++20 $(WARNINGS)
RELEASE_FLAGS ?= -O3 -DNDEBUG
DEBUG_FLAGS ?= -O1 -g -fsanitize=address,undefined
LDFLAGS ?=

SOURCES := src/main.cpp
BUILD_DIR := build
RELEASE_BIN := bubu
DEBUG_BIN := $(BUILD_DIR)/bubu-debug

.PHONY: all release debug test bench clean

all: release

release: $(RELEASE_BIN)

$(RELEASE_BIN): $(SOURCES) | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(COMMON_FLAGS) $(RELEASE_FLAGS) $(SOURCES) $(LDFLAGS) -o $@

debug: $(DEBUG_BIN)

$(DEBUG_BIN): $(SOURCES) | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(COMMON_FLAGS) $(DEBUG_FLAGS) $(SOURCES) $(LDFLAGS) -o $@

$(BUILD_DIR):
	mkdir -p $@

test: release
	./$(RELEASE_BIN) --self-check

bench: release
	./$(RELEASE_BIN) bench

clean:
	rm -rf $(BUILD_DIR) $(RELEASE_BIN)

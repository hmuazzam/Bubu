#pragma once

#include "search.h"

#include <cstdint>

namespace bubu {

struct BenchResult {
    std::uint64_t nodes = 0;
    std::uint64_t time_ms = 0;
    std::uint64_t nps = 0;
};

BenchResult run_bench(Searcher& searcher, int depth);

}  // namespace bubu

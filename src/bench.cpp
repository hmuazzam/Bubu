#include "bench.h"

#include "position.h"

#include <algorithm>
#include <chrono>

namespace bubu {

BenchResult run_bench(Searcher& searcher, int depth) {
    constexpr const char* positions[] = {
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
        "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
        "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1"};
    const auto start = std::chrono::steady_clock::now();
    std::uint64_t nodes = 0;
    for (const char* fen : positions) {
        Position position;
        if (!position.set_fen(fen)) return {};
        searcher.clear_hash();
        SearchLimits limits;
        limits.depth = depth;
        nodes += searcher.search(position, limits, [](const SearchInfo&) {}).nodes;
    }
    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                             std::chrono::steady_clock::now() - start)
                             .count();
    const std::uint64_t time_ms = static_cast<std::uint64_t>(std::max<std::int64_t>(1, elapsed));
    return {nodes, time_ms, nodes * 1000 / time_ms};
}

}  // namespace bubu

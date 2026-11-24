// Offline generator for Bubu's sliding-attack multipliers. The values are
// generated from a fixed local PRNG seed; no engine tables are imported.
#include <array>
#include <bit>
#include <cstdint>
#include <iostream>
#include <vector>

namespace {

using U64 = std::uint64_t;
std::uint64_t random_state = 0xB0B0C0FFEE123456ULL;

U64 random_u64() {
    random_state ^= random_state >> 12;
    random_state ^= random_state << 25;
    random_state ^= random_state >> 27;
    return random_state * 0x2545F4914F6CDD1DULL;
}

U64 sparse_random() { return random_u64() & random_u64() & random_u64(); }
int file_of(int square) { return square & 7; }
int rank_of(int square) { return square >> 3; }

U64 attacks_for(int square, U64 occupied, bool bishop) {
    constexpr int diagonal[4][2] = {{1, 1}, {-1, 1}, {1, -1}, {-1, -1}};
    constexpr int straight[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
    U64 attacks = 0;
    const int (*dirs)[2] = bishop ? diagonal : straight;
    const int count = 4;
    for (int direction = 0; direction < count; ++direction) {
        int file = file_of(square) + dirs[direction][0];
        int rank = rank_of(square) + dirs[direction][1];
        while (file >= 0 && file < 8 && rank >= 0 && rank < 8) {
            const int target = rank * 8 + file;
            attacks |= U64{1} << target;
            if ((occupied & (U64{1} << target)) != 0) break;
            file += dirs[direction][0];
            rank += dirs[direction][1];
        }
    }
    return attacks;
}

U64 relevant_mask(int square, bool bishop) {
    constexpr int diagonal[4][2] = {{1, 1}, {-1, 1}, {1, -1}, {-1, -1}};
    constexpr int straight[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
    U64 mask = 0;
    const int (*dirs)[2] = bishop ? diagonal : straight;
    for (int direction = 0; direction < 4; ++direction) {
        int file = file_of(square) + dirs[direction][0];
        int rank = rank_of(square) + dirs[direction][1];
        while (file >= 0 && file < 8 && rank >= 0 && rank < 8) {
            const int next_file = file + dirs[direction][0];
            const int next_rank = rank + dirs[direction][1];
            if (next_file < 0 || next_file >= 8 || next_rank < 0 || next_rank >= 8)
                break;
            mask |= U64{1} << (rank * 8 + file);
            file = next_file;
            rank = next_rank;
        }
    }
    return mask;
}

std::vector<U64> occupancies(U64 mask) {
    std::vector<int> bits;
    while (mask != 0) {
        const int bit = std::countr_zero(mask);
        bits.push_back(bit);
        mask &= mask - 1;
    }
    std::vector<U64> result(1ULL << bits.size());
    for (std::size_t subset = 0; subset < result.size(); ++subset)
        for (std::size_t bit = 0; bit < bits.size(); ++bit)
            if ((subset & (1ULL << bit)) != 0) result[subset] |= U64{1} << bits[bit];
    return result;
}

U64 find_magic(int square, bool bishop) {
    const U64 mask = relevant_mask(square, bishop);
    const int shift = 64 - std::popcount(mask);
    const std::vector<U64> blockers = occupancies(mask);
    std::vector<U64> attacks;
    attacks.reserve(blockers.size());
    for (U64 blocker : blockers) attacks.push_back(attacks_for(square, blocker, bishop));

    std::array<U64, 4096> values{};
    std::array<unsigned, 4096> stamps{};
    unsigned generation = 0;
    for (;;) {
        const U64 candidate = sparse_random();
        if (std::popcount((mask * candidate) & 0xFF00000000000000ULL) < 6) continue;
        ++generation;
        bool valid = true;
        for (std::size_t index = 0; index < blockers.size(); ++index) {
            const std::size_t slot = (blockers[index] * candidate) >> shift;
            if (stamps[slot] != generation) {
                stamps[slot] = generation;
                values[slot] = attacks[index];
            } else if (values[slot] != attacks[index]) {
                valid = false;
                break;
            }
        }
        if (valid) return candidate;
    }
}

}  // namespace

int main() {
    std::cout << "#pragma once\n#include <array>\n#include <cstdint>\n\n"
                 "namespace bubu {\nusing Magic = std::uint64_t;\n";
    for (bool bishop : {true, false}) {
        std::cout << "inline constexpr std::array<Magic, 64> "
                  << (bishop ? "bishop" : "rook") << "_magics = {\n";
        for (int square = 0; square < 64; ++square) {
            const U64 magic = find_magic(square, bishop);
            std::cout << "0x" << std::hex << magic << "ULL"
                      << (square == 63 ? "\n" : ",\n");
        }
        std::cout << "};\n";
    }
    std::cout << "}  // namespace bubu\n";
}

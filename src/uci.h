#pragma once

#include "position.h"
#include "search.h"

#include <atomic>
#include <mutex>
#include <string>
#include <thread>

namespace bubu {

class UciEngine {
public:
    UciEngine();
    ~UciEngine();
    int run();

private:
    Position position_;
    Searcher searcher_;
    std::thread worker_;
    std::atomic<bool> searching_{false};
    std::mutex output_mutex_;
    int hash_mb_ = 16;
    bool king_safety_v2_ = true;

    void write_line(const std::string& line);
    void stop_and_join();
    void handle_position(const std::string& command);
    void handle_go(const std::string& command);
    void handle_setoption(const std::string& command);
    void print_info(const SearchInfo& info);
};

}  // namespace bubu

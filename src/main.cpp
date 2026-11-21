#include <iostream>
#include <string_view>

int main(int argc, char* argv[]) {
    if (argc > 1 && std::string_view(argv[1]) == "--self-check") {
        std::cout << "Bubu scaffold self-check: ok\n";
        return 0;
    }

    if (argc > 1 && std::string_view(argv[1]) == "bench") {
        std::cout << "bench unavailable: search is introduced in milestone M4\n";
        return 0;
    }

    std::cout << "Bubu chess engine scaffold\n";
    return 0;
}

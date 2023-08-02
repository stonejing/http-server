#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;

int main() {
    std::string current_path = fs::current_path().string();
    std::cout << "Current Path: " << current_path << std::endl;

    for (const auto& entry : fs::directory_iterator(current_path)) {
        if (fs::is_directory(entry)) {
            std::cout << "[Directory] " << entry.path().filename().string() << std::endl;
        } else if (fs::is_regular_file(entry)) {
            std::cout << "[File] " << entry.path().filename().string() << std::endl;
        }
    }

    return 0;
}
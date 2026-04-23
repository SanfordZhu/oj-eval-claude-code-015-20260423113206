#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <string>
#include <sstream>
#include <filesystem>
#include <cstdio>

namespace fs = std::filesystem;

class FileStorage {
private:
    std::string base_dir;

    std::string getSafeFilename(const std::string& index) {
        // Create a safe filename from index
        std::string safe = index;
        // Replace any potentially problematic characters
        for (char& c : safe) {
            if (c == '/' || c == '\\' || c == '.' || c == ' ' || c == '\n' || c == '\r' || c == '\t') {
                c = '_';
            }
        }
        // Limit length to avoid filesystem issues
        if (safe.length() > 200) {
            safe = safe.substr(0, 200);
        }
        return base_dir + "/" + safe + ".dat";
    }

    void ensureDirectoryExists() {
        if (!fs::exists(base_dir)) {
            fs::create_directory(base_dir);
        }
    }

public:
    FileStorage() {
        base_dir = "storage";
        ensureDirectoryExists();
    }

    void insert(const std::string& index, int value) {
        std::string filename = getSafeFilename(index);
        std::vector<int> values;

        // Read existing values if file exists
        std::ifstream infile(filename, std::ios::binary);
        if (infile.is_open()) {
            int val;
            while (infile.read(reinterpret_cast<char*>(&val), sizeof(int))) {
                values.push_back(val);
            }
            infile.close();
        }

        // Check if value already exists
        auto it = std::lower_bound(values.begin(), values.end(), value);
        if (it != values.end() && *it == value) {
            return; // Value already exists
        }

        // Insert and keep sorted
        values.insert(it, value);

        // Write back to file
        std::ofstream outfile(filename, std::ios::binary | std::ios::trunc);
        if (outfile.is_open()) {
            for (int val : values) {
                outfile.write(reinterpret_cast<const char*>(&val), sizeof(int));
            }
            outfile.close();
        }
    }

    void remove(const std::string& index, int value) {
        std::string filename = getSafeFilename(index);

        // If file doesn't exist, nothing to delete
        if (!fs::exists(filename)) {
            return;
        }

        std::vector<int> values;

        // Read all values except the one to delete
        std::ifstream infile(filename, std::ios::binary);
        if (infile.is_open()) {
            int val;
            while (infile.read(reinterpret_cast<char*>(&val), sizeof(int))) {
                if (val != value) {
                    values.push_back(val);
                }
            }
            infile.close();
        }

        if (values.empty()) {
            // Delete the file if no values left
            std::remove(filename.c_str());
        } else {
            // Write back remaining values
            std::ofstream outfile(filename, std::ios::binary | std::ios::trunc);
            if (outfile.is_open()) {
                for (int val : values) {
                    outfile.write(reinterpret_cast<const char*>(&val), sizeof(int));
                }
                outfile.close();
            }
        }
    }

    std::vector<int> find(const std::string& index) {
        std::string filename = getSafeFilename(index);
        std::vector<int> result;

        // If file doesn't exist, return empty
        if (!fs::exists(filename)) {
            return result;
        }

        // Read all values
        std::ifstream infile(filename, std::ios::binary);
        if (infile.is_open()) {
            int val;
            while (infile.read(reinterpret_cast<char*>(&val), sizeof(int))) {
                result.push_back(val);
            }
            infile.close();
        }

        return result;
    }
};

int main() {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    FileStorage storage;
    int n;
    std::cin >> n;
    std::cin.ignore();

    for (int i = 0; i < n; i++) {
        std::string line;
        std::getline(std::cin, line);
        std::istringstream iss(line);

        std::string command;
        iss >> command;

        if (command == "insert") {
            std::string index;
            int value;
            iss >> index >> value;
            storage.insert(index, value);
        } else if (command == "delete") {
            std::string index;
            int value;
            iss >> index >> value;
            storage.remove(index, value);
        } else if (command == "find") {
            std::string index;
            iss >> index;
            std::vector<int> values = storage.find(index);

            if (values.empty()) {
                std::cout << "null\n";
            } else {
                for (size_t j = 0; j < values.size(); j++) {
                    if (j > 0) std::cout << " ";
                    std::cout << values[j];
                }
                std::cout << "\n";
            }
        }
    }

    return 0;
}
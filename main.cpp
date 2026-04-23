#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <string>
#include <filesystem>
#include <sstream>

namespace fs = std::filesystem;

class FileStorage {
private:
    std::string base_dir;

    std::string getFilename(const std::string& index) {
        // Create a safe filename from index
        std::string safe_index = index;
        // Replace any potentially problematic characters
        for (char& c : safe_index) {
            if (c == '/' || c == '\\' || c == '.' || c == ' ') {
                c = '_';
            }
        }
        return base_dir + "/" + safe_index + ".dat";
    }

public:
    FileStorage() {
        base_dir = "storage";
        // Create storage directory if it doesn't exist
        if (!fs::exists(base_dir)) {
            fs::create_directory(base_dir);
        }
    }

    void insert(const std::string& index, int value) {
        std::string filename = getFilename(index);
        std::vector<int> values;

        // Read existing values if file exists
        if (fs::exists(filename)) {
            std::ifstream infile(filename, std::ios::binary);
            if (infile.is_open()) {
                int val;
                while (infile.read(reinterpret_cast<char*>(&val), sizeof(int))) {
                    values.push_back(val);
                }
                infile.close();
            }
        }

        // Check if value already exists
        if (std::binary_search(values.begin(), values.end(), value)) {
            return; // Value already exists, don't insert duplicate
        }

        // Insert and sort
        values.push_back(value);
        std::sort(values.begin(), values.end());

        // Write back to file
        std::ofstream outfile(filename, std::ios::binary);
        if (outfile.is_open()) {
            for (int val : values) {
                outfile.write(reinterpret_cast<const char*>(&val), sizeof(int));
            }
            outfile.close();
        }
    }

    void remove(const std::string& index, int value) {
        std::string filename = getFilename(index);

        // If file doesn't exist, nothing to delete
        if (!fs::exists(filename)) {
            return;
        }

        std::vector<int> values;

        // Read all values
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

        // Write back remaining values
        if (values.empty()) {
            // Delete the file if no values left
            fs::remove(filename);
        } else {
            std::ofstream outfile(filename, std::ios::binary);
            if (outfile.is_open()) {
                for (int val : values) {
                    outfile.write(reinterpret_cast<const char*>(&val), sizeof(int));
                }
                outfile.close();
            }
        }
    }

    std::vector<int> find(const std::string& index) {
        std::string filename = getFilename(index);
        std::vector<int> values;

        // If file doesn't exist, return empty vector
        if (!fs::exists(filename)) {
            return values;
        }

        // Read all values
        std::ifstream infile(filename, std::ios::binary);
        if (infile.is_open()) {
            int val;
            while (infile.read(reinterpret_cast<char*>(&val), sizeof(int))) {
                values.push_back(val);
            }
            infile.close();
        }

        return values;
    }
};

int main() {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    FileStorage storage;
    int n;
    std::cin >> n;
    std::cin.ignore(); // Consume newline

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
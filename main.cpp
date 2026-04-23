#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <string>
#include <sstream>
#include <functional>

class FileStorage {
private:
    static constexpr int NUM_BUCKETS = 100;  // Fixed number of files
    static constexpr const char* BASE_DIR = "storage";

    int getBucket(const std::string& index) {
        std::hash<std::string> hasher;
        return hasher(index) % NUM_BUCKETS;
    }

    std::string getBucketFilename(int bucket) {
        return std::string(BASE_DIR) + "/bucket_" + std::to_string(bucket) + ".dat";
    }

    void ensureDirectoryExists() {
        std::ifstream dir_check(BASE_DIR);
        if (!dir_check.good()) {
            system("mkdir -p storage");
        }
        dir_check.close();
    }

public:
    FileStorage() {
        ensureDirectoryExists();
    }

    void insert(const std::string& index, int value) {
        int bucket = getBucket(index);
        std::string filename = getBucketFilename(bucket);

        // Read all data from the bucket
        std::vector<std::pair<std::string, std::vector<int>>> bucket_data;

        std::ifstream infile(filename, std::ios::binary);
        if (infile.is_open()) {
            while (!infile.eof()) {
                // Read index length
                size_t index_len;
                infile.read(reinterpret_cast<char*>(&index_len), sizeof(index_len));
                if (infile.eof()) break;

                // Read index
                std::string idx(index_len, '\0');
                infile.read(&idx[0], index_len);

                // Read value count
                size_t value_count;
                infile.read(reinterpret_cast<char*>(&value_count), sizeof(value_count));

                // Read values
                std::vector<int> values;
                for (size_t i = 0; i < value_count; i++) {
                    int val;
                    infile.read(reinterpret_cast<char*>(&val), sizeof(int));
                    values.push_back(val);
                }

                bucket_data.push_back({idx, values});
            }
            infile.close();
        }

        // Find and update the index
        bool found = false;
        for (auto& [idx, values] : bucket_data) {
            if (idx == index) {
                // Check if value already exists
                auto it = std::lower_bound(values.begin(), values.end(), value);
                if (it != values.end() && *it == value) {
                    return; // Value already exists
                }
                values.insert(it, value);
                found = true;
                break;
            }
        }

        // If not found, add new entry
        if (!found) {
            bucket_data.push_back({index, {value}});
        }

        // Write back to file
        std::ofstream outfile(filename, std::ios::binary | std::ios::trunc);
        if (outfile.is_open()) {
            for (const auto& [idx, values] : bucket_data) {
                // Write index length
                size_t index_len = idx.length();
                outfile.write(reinterpret_cast<const char*>(&index_len), sizeof(index_len));

                // Write index
                outfile.write(idx.c_str(), index_len);

                // Write value count
                size_t value_count = values.size();
                outfile.write(reinterpret_cast<const char*>(&value_count), sizeof(value_count));

                // Write values
                for (int val : values) {
                    outfile.write(reinterpret_cast<const char*>(&val), sizeof(int));
                }
            }
            outfile.close();
        }
    }

    void remove(const std::string& index, int value) {
        int bucket = getBucket(index);
        std::string filename = getBucketFilename(bucket);

        // Read all data from the bucket
        std::vector<std::pair<std::string, std::vector<int>>> bucket_data;

        std::ifstream infile(filename, std::ios::binary);
        if (infile.is_open()) {
            while (!infile.eof()) {
                // Read index length
                size_t index_len;
                infile.read(reinterpret_cast<char*>(&index_len), sizeof(index_len));
                if (infile.eof()) break;

                // Read index
                std::string idx(index_len, '\0');
                infile.read(&idx[0], index_len);

                // Read value count
                size_t value_count;
                infile.read(reinterpret_cast<char*>(&value_count), sizeof(value_count));

                // Read values
                std::vector<int> values;
                for (size_t i = 0; i < value_count; i++) {
                    int val;
                    infile.read(reinterpret_cast<char*>(&val), sizeof(int));
                    if (val != value || idx != index) {
                        values.push_back(val);
                    }
                }

                // Only keep non-empty indices
                if (!values.empty() || idx != index) {
                    if (idx == index && !values.empty()) {
                        bucket_data.push_back({idx, values});
                    } else if (idx != index) {
                        bucket_data.push_back({idx, values});
                    }
                }
            }
            infile.close();
        }

        // Write back to file
        std::ofstream outfile(filename, std::ios::binary | std::ios::trunc);
        if (outfile.is_open()) {
            for (const auto& [idx, values] : bucket_data) {
                // Write index length
                size_t index_len = idx.length();
                outfile.write(reinterpret_cast<const char*>(&index_len), sizeof(index_len));

                // Write index
                outfile.write(idx.c_str(), index_len);

                // Write value count
                size_t value_count = values.size();
                outfile.write(reinterpret_cast<const char*>(&value_count), sizeof(value_count));

                // Write values
                for (int val : values) {
                    outfile.write(reinterpret_cast<const char*>(&val), sizeof(int));
                }
            }
            outfile.close();
        }
    }

    std::vector<int> find(const std::string& index) {
        int bucket = getBucket(index);
        std::string filename = getBucketFilename(bucket);

        std::vector<int> result;

        std::ifstream infile(filename, std::ios::binary);
        if (infile.is_open()) {
            while (!infile.eof()) {
                // Read index length
                size_t index_len;
                infile.read(reinterpret_cast<char*>(&index_len), sizeof(index_len));
                if (infile.eof()) break;

                // Read index
                std::string idx(index_len, '\0');
                infile.read(&idx[0], index_len);

                // Read value count
                size_t value_count;
                infile.read(reinterpret_cast<char*>(&value_count), sizeof(value_count));

                // Read values
                std::vector<int> values;
                for (size_t i = 0; i < value_count; i++) {
                    int val;
                    infile.read(reinterpret_cast<char*>(&val), sizeof(int));
                    if (idx == index) {
                        result.push_back(val);
                    }
                }

                if (idx == index) {
                    break;
                }
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
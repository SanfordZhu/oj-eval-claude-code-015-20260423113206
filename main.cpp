#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <string>
#include <unordered_map>
#include <sstream>
#include <cstring>
#include <cstdint>

class FileStorage {
private:
    static constexpr const char* DATA_FILE = "storage/data.db";
    static constexpr const char* INDEX_FILE = "storage/index.db";
    static constexpr int BUFFER_SIZE = 4096;

    // Structure to store index information
    struct IndexInfo {
        uint32_t offset;  // Offset in data file
        uint32_t count;   // Number of values
    };

    std::unordered_map<std::string, IndexInfo> index_cache;
    bool cache_dirty;

    void ensureDirectoryExists() {
        std::ifstream dir_check("storage");
        if (!dir_check.good()) {
            system("mkdir -p storage");
        }
        dir_check.close();
    }

    void loadIndex() {
        std::ifstream idx_file(INDEX_FILE, std::ios::binary);
        if (!idx_file.is_open()) return;

        uint32_t index_count;
        idx_file.read(reinterpret_cast<char*>(&index_count), sizeof(index_count));

        for (uint32_t i = 0; i < index_count; i++) {
            uint32_t key_len;
            idx_file.read(reinterpret_cast<char*>(&key_len), sizeof(key_len));

            std::string key(key_len, '\0');
            idx_file.read(&key[0], key_len);

            IndexInfo info;
            idx_file.read(reinterpret_cast<char*>(&info.offset), sizeof(info.offset));
            idx_file.read(reinterpret_cast<char*>(&info.count), sizeof(info.count));

            index_cache[key] = info;
        }
        idx_file.close();
        cache_dirty = false;
    }

    void saveIndex() {
        if (!cache_dirty) return;

        std::ofstream idx_file(INDEX_FILE, std::ios::binary);
        if (!idx_file.is_open()) return;

        uint32_t index_count = index_cache.size();
        idx_file.write(reinterpret_cast<const char*>(&index_count), sizeof(index_count));

        for (const auto& [key, info] : index_cache) {
            uint32_t key_len = key.length();
            idx_file.write(reinterpret_cast<const char*>(&key_len), sizeof(key_len));
            idx_file.write(key.c_str(), key_len);
            idx_file.write(reinterpret_cast<const char*>(&info.offset), sizeof(info.offset));
            idx_file.write(reinterpret_cast<const char*>(&info.count), sizeof(info.count));
        }
        idx_file.close();
        cache_dirty = false;
    }

public:
    FileStorage() : cache_dirty(false) {
        ensureDirectoryExists();
        loadIndex();
    }

    ~FileStorage() {
        saveIndex();
    }

    void insert(const std::string& index, int value) {
        // Load existing values
        std::vector<int> values;
        if (index_cache.find(index) != index_cache.end()) {
            const IndexInfo& info = index_cache[index];
            std::ifstream data_file(DATA_FILE, std::ios::binary);
            if (data_file.is_open()) {
                data_file.seekg(info.offset);
                for (uint32_t i = 0; i < info.count; i++) {
                    int val;
                    data_file.read(reinterpret_cast<char*>(&val), sizeof(int));
                    values.push_back(val);
                }
                data_file.close();
            }
        }

        // Check for duplicate
        if (std::binary_search(values.begin(), values.end(), value)) {
            return;
        }

        // Insert and sort
        values.push_back(value);
        std::sort(values.begin(), values.end());

        // Write back to file
        std::fstream data_file(DATA_FILE, std::ios::in | std::ios::out | std::ios::binary);
        if (!data_file.is_open()) {
            data_file.clear();
            data_file.open(DATA_FILE, std::ios::out | std::ios::binary);
            data_file.close();
            data_file.open(DATA_FILE, std::ios::in | std::ios::out | std::ios::binary);
        }

        // Find position to write
        uint32_t new_offset = 0;
        if (!index_cache.empty()) {
            // Find the last index and calculate new offset
            auto it = index_cache.begin();
            uint32_t max_offset = 0;
            uint32_t max_count = 0;

            for (const auto& [key, info] : index_cache) {
                if (info.offset + info.count * sizeof(int) > max_offset) {
                    max_offset = info.offset + info.count * sizeof(int);
                }
            }
            new_offset = max_offset;
        }

        // If index already exists, we need to reorganize
        if (index_cache.find(index) != index_cache.end()) {
            // For simplicity, append at the end (could be optimized)
            IndexInfo old_info = index_cache[index];

            // Read all data after this index
            std::vector<std::pair<std::string, std::vector<int>>> all_data;

            // First, collect all data including the modified one
            for (const auto& [key, info] : index_cache) {
                if (key != index) {
                    std::vector<int> vals;
                    data_file.seekg(info.offset);
                    for (uint32_t i = 0; i < info.count; i++) {
                        int val;
                        data_file.read(reinterpret_cast<char*>(&val), sizeof(int));
                        vals.push_back(val);
                    }
                    all_data.push_back({key, vals});
                } else {
                    all_data.push_back({index, values});
                }
            }

            // Clear the file and rewrite everything
            data_file.close();
            data_file.open(DATA_FILE, std::ios::out | std::ios::trunc | std::ios::binary);

            uint32_t offset = 0;
            for (auto& [key, vals] : all_data) {
                index_cache[key].offset = offset;
                index_cache[key].count = vals.size();

                for (int val : vals) {
                    data_file.write(reinterpret_cast<const char*>(&val), sizeof(int));
                }
                offset += vals.size() * sizeof(int);
            }
            cache_dirty = true;
        } else {
            // New index, append at the end
            data_file.seekp(new_offset);
            for (int val : values) {
                data_file.write(reinterpret_cast<const char*>(&val), sizeof(int));
            }

            index_cache[index].offset = new_offset;
            index_cache[index].count = values.size();
            cache_dirty = true;
        }

        data_file.close();
    }

    void remove(const std::string& index, int value) {
        if (index_cache.find(index) == index_cache.end()) {
            return;
        }

        const IndexInfo& info = index_cache[index];
        std::vector<int> values;

        // Read existing values
        std::ifstream data_file(DATA_FILE, std::ios::binary);
        if (data_file.is_open()) {
            data_file.seekg(info.offset);
            for (uint32_t i = 0; i < info.count; i++) {
                int val;
                data_file.read(reinterpret_cast<char*>(&val), sizeof(int));
                if (val != value) {
                    values.push_back(val);
                }
            }
            data_file.close();
        }

        if (values.empty()) {
            // Remove the index entirely
            index_cache.erase(index);
            cache_dirty = true;

            // Reorganize file (simplified approach)
            reorganizeFile();
        } else {
            // Update with remaining values
            std::fstream data_file(DATA_FILE, std::ios::in | std::ios::out | std::ios::binary);
            if (data_file.is_open()) {
                data_file.seekp(info.offset);
                for (int val : values) {
                    data_file.write(reinterpret_cast<const char*>(&val), sizeof(int));
                }
                index_cache[index].count = values.size();
                cache_dirty = true;
                data_file.close();
            }
        }
    }

    void reorganizeFile() {
        // Collect all data
        std::vector<std::pair<std::string, std::vector<int>>> all_data;

        std::ifstream in_file(DATA_FILE, std::ios::binary);
        if (in_file.is_open()) {
            for (const auto& [key, info] : index_cache) {
                std::vector<int> vals;
                in_file.seekg(info.offset);
                for (uint32_t i = 0; i < info.count; i++) {
                    int val;
                    in_file.read(reinterpret_cast<char*>(&val), sizeof(int));
                    vals.push_back(val);
                }
                all_data.push_back({key, vals});
            }
            in_file.close();
        }

        // Rewrite file
        std::ofstream out_file(DATA_FILE, std::ios::binary | std::ios::trunc);
        if (out_file.is_open()) {
            uint32_t offset = 0;
            for (auto& [key, vals] : all_data) {
                index_cache[key].offset = offset;
                index_cache[key].count = vals.size();

                for (int val : vals) {
                    out_file.write(reinterpret_cast<const char*>(&val), sizeof(int));
                }
                offset += vals.size() * sizeof(int);
            }
            out_file.close();
        }
    }

    std::vector<int> find(const std::string& index) {
        std::vector<int> result;

        if (index_cache.find(index) == index_cache.end()) {
            return result;
        }

        const IndexInfo& info = index_cache[index];
        std::ifstream data_file(DATA_FILE, std::ios::binary);
        if (data_file.is_open()) {
            data_file.seekg(info.offset);
            for (uint32_t i = 0; i < info.count; i++) {
                int val;
                data_file.read(reinterpret_cast<char*>(&val), sizeof(int));
                result.push_back(val);
            }
            data_file.close();
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
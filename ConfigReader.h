#pragma once
#include <string>
#include <vector>
#include <fstream>  
#include <sstream>  
#include <stdexcept>
#include <utility>  

std::vector<std::pair<std::string, std::string>> read_kv_pairs(const std::string& file_path) {
    std::vector<std::pair<std::string, std::string>> result;
    std::ifstream file(file_path);
    if (!file) {
        throw std::runtime_error("Could not open file: " + file_path);
    }
    std::string line;
    while (std::getline(file, line)) {
        std::istringstream ss(line);
        std::string key, value;
        if (std::getline(ss, key, ',') && std::getline(ss, value)) {
            result.emplace_back(key, value);
        }
    }
    return result;
}
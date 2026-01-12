#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

class Context {
public:
    std::unordered_map<std::string, uint64_t> var_mapper;
    uint64_t var_allocer = 0;
    std::vector<std::string> emission;

    uint64_t var(std::string& var_name);

    void emit(std::string line);

    std::string render();
};
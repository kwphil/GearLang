#include <unordered_map>
#include <string>
#include <vector>

#include "ctx.hpp"

uint64_t Context::var(std::string& var_name) {
    if (var_mapper.find(var_name) == var_mapper.end())
        var_mapper[var_name] = (var_allocer++);

    return var_mapper[var_name];
}

void Context::emit(std::string line) {
    emission.push_back(line);
}

std::string Context::render() {
    std::string out;

    out += "format ELF64 executable\n";
    out += "entry start\n";
    out += "    vars: rq " + std::to_string(var_allocer) + "\n";
    out += "segment readable executable\n";
    out += "start:\n";

    for (const auto& line : emission)
        out += line + "\n";

    out += "mov rdi, rax\n";
    out += "mov rax, 60\n"; //syscall exit
    out += "syscall\n";

    return out;
}
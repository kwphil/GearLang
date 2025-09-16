
#include <unordered_map>
#include <string>
#include <vector>

class Context
{
   
public:
    std::unordered_map<std::string, uint64_t> var_mapper;
    uint64_t var_allocer = 0;
    std::vector<std::string> emission;

    uint64_t var(std::string& var_name)
    {
        if (var_mapper.find(var_name) == var_mapper.end())
            var_mapper[var_name] = (var_allocer++);

        return var_mapper[var_name];
    }

    void emit(std::string line)
    {
        emission.push_back(line);
    }

    std::string render()
    {
        std::string out;

        out += "format ELF executable\n";
        out += "entry start\n";
        out += "section '.data' writable\n";
        out += "    vars: rq " + std::to_string(var_allocer) + "\n";
        out += "section '.text' writable\n";
        out += "start:\n";

        for (const auto& line : emission)
            out += line + "\n";

        out += "ret\n";

        return out;

    }



};




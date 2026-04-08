#include <vector>
#include <string>

#include <gearlang/etc.hpp>
#include <gearlang/error.hpp>

struct LogEntry {
    Error::ErrorCodes code; 
    unordered_map<std::string, std::string> keys;
    Span location;
};

class Logger {
private:
    std::vector<LogEntry> entries;

public:
    inline void log(
        unordered_map<std::string, std::string>& keys,
        Error::ErrorCodes code,
        Span loc
    ) {
        entries.push_back({code, keys, loc});
    }

    inline std::string to_json() const {
        std::ostringstream out;
        out << "{\n  \"diagnostics\": [\n";

        for (size_t i = 0; i < entries.size(); ++i) {
            const auto& e = entries[i];

            out << "    {\n";
            for(auto entry : e.keys) {
                out << "      \"" << entry.first << "\": \"" << entry.second << "\"\n";
            }
            out << "      \"code\": \"" << (int)e.code << "\",\n";
            out << "      \"location\": {\n";
            out << "        \"file\": \"" << escape(e.location.file) << "\",\n";
            out << "        \"line\": " << e.location.line << ",\n";
            out << "        \"column\": " << e.location.col << "\n";
            out << "      }\n";
            out << "    }";

            if (i != entries.size() - 1) out << ",";
            out << "\n";
        }

        out << "  ]\n}";
        return out.str();
    }

private:
    inline std::string escape(const std::string& s) const {
        std::ostringstream o;
        for (char c : s) {
            switch (c) {
                case '"': o << "\\\""; break;
                case '\\': o << "\\\\"; break;
                case '\n': o << "\\n"; break;
                case '\t': o << "\\t"; break;
                default: o << c;
            }
        }
        return o.str();
    }
};
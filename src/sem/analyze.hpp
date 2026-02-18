#include <unordered_map>
#include <string>

#include "../lex.hpp"
#include "../value.hpp"

namespace Sem {
    class Analyzer {
    private:
    
        // ----------- DECLARATIONS --------------
        typedef struct {
            std::unordered_map<std::string, Sem::Variable> variables;

        } Scope;

        std::unordered_map<Sem::Variable>

        // ----------- TYPE

    public:
        void analyze(Lexer::Stream& s);
    };
}
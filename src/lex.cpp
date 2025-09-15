
#include <string>
#include <vector>
#include <iostream>
#include <fstream>

namespace Lexer
{
    enum Types
    {
        Keyword,
        Iden,
        IntLit,
        FloatLit,
        Operator,
        Symbol,
        ParenOpen,
        ParenClose,
    };


    class Token
    {
        std::string content;
        Types type;
    };
        
    typedef std::vector<Token> Stream;

    Stream tokenize(std::string& source_path)
    {
        std::ifstream file(source_path);
        Stream out;

        char c;
        while (file >> c)
        {
            std::cout << c << '\n';
        }
        
        return out;
    }
    


}







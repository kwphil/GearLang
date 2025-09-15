
#include <cctype>
#include <iostream>
#include <string>
#include <unordered_set>
#include <vector>
#include <fstream>
#include <stdint.h>
#include <iostream>

namespace Lexer
{


//finite state machine
enum class CharType
{
    Invalid,
    Alpha,
    Num,
    Paren,
    Sym,
    Format,
};

CharType getCharType(char c)
{
    if (isalpha(c)            ) return CharType::Alpha;
    if (isdigit(c) || c == '.') return CharType::Num;
    if (c == '('   || c == ')') return CharType::Paren;
    if (c == ' '   || c == '\n' || c == '\t') return CharType::Format;
    return CharType::Sym;
}



enum class Type
{
    Invalid,

    Keyword,
    Identifier,
    IntegerLiteral,
    FloatLiteral, 
    Operator,
    ParenOpen,
    ParenClose,
};

Type classify(std::string& content, CharType state)
{
    static const std::unordered_set<std::string> keywords = {
        "fn", "let", "comptime", "assert", "test"
    };


    switch (state)
    {
        case CharType::Invalid: [[fallthrough]];
        case CharType::Format: break; //unreachable
        
        case CharType::Alpha:
            return 
                keywords.find(content) == keywords.end()
                ? Type::Identifier : Type::Keyword;

        case CharType::Paren:
            if (content == "(") return Type::ParenOpen;
            if (content == ")") return Type::ParenClose;
            break;

        case CharType::Num:
            if (content.find('.') == std::string::npos) return Type::IntegerLiteral;
            return Type::FloatLiteral;

        case CharType::Sym:
            //TODO implement this
            return Type::Invalid; 
    }   


    std::cerr << "This should not be reached!!!!" << '\n' << std::flush;
}



class Token
{
public:
    std::string content;
    uint32_t line;
    Type type;

};


class Stream
{
public:
    std::vector<Token> content;
    uint32_t index = 0;

};





Stream tokenize(std::string& source_path)
{
    std::ifstream file(source_path);
    Stream out;

    char c;
    uint32_t line = 1;
    CharType state_new, state_old = CharType::Invalid;
    Token tok = Token();
    while (file.get(c))
    {
        state_new = getCharType(c);

        std::cout << (int)c << '\n';

        //state transition -> token boundary
        if (state_new != state_old)
        {
            //invalid state should not emit
            if (state_old != CharType::Invalid && state_old != CharType::Format)
            {
                tok.type = classify(tok.content, state_old);
                tok.line = line;

                out.content.push_back(tok);

                tok = Token();
            }
            else
                tok.content.clear();
        }


        if (c == '\n') line++;
        
        tok.content += c;
        state_old = state_new;
    }
    
    return out;
}





}








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
    Quote,
};

CharType getCharType(char c)
{
    if (isalpha(c) || c == '_') return CharType::Alpha;
    if (isdigit(c) || c == '.') return CharType::Num;
    if (c == '('   || c == ')') return CharType::Paren;
    if (c == ' '   || c == '\n' || c == '\t') return CharType::Format;
    if (c == '"'              ) return CharType::Quote;
    return CharType::Sym;
}



enum class Type
{
    Invalid,

    Keyword,
    Identifier,
    IntegerLiteral,
    FloatLiteral, 
    StringLiteral,
    Operator,
    ParenOpen,
    ParenClose,
};

Type classify(std::string& content, CharType state)
{
    static const std::unordered_set<std::string> keywords = {
        "fn", "let", "comptime", "assert", "test"
    };
    static const std::unordered_set<std::string> operators = {
        "+", "-", "*", "/",
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
            if (operators.find(content) != operators.end())
                return Type::Operator;
            
            //TODO implement this
            return Type::Invalid; 

        case CharType::Quote:
            return Type::StringLiteral;
    }   


    std::cerr << "This should not be reached!!!!" << '\n' << std::flush;
    return Type::Invalid;
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
private:
    uint32_t index = 0;
public:
    std::vector<Token> content;

    bool has()
    {
        return (index != content.size());
    }

    Token peek()
    {
        return content[index];
    }

    Token pop()
    {
        return content[index++];
    }

    void expect(const char* should)
    {
        auto is = pop().content;
        if (is != should)
        {
            std::cerr << "Error: Expected '" << should << "', but got '" << is << "'\n";
        }
    }


};





Stream tokenize(std::string& source_path)
{
    std::ifstream file(source_path);
    Stream out;

    char c;
    uint32_t line = 1;
    CharType state_new, state_old = CharType::Invalid;
    Token tok = Token();

    bool is_string = false;
    bool is_comment = false;

    while (file.get(c))
    {
        state_new = getCharType(c);

        //state transition -> token boundary
        if ((state_new != state_old) && !is_string)
        {
            //transition cannot happen from alpha to num,
            //because of identifiers like 'num1'
            if (
                state_new == CharType::Num &&
                state_old == CharType::Alpha
            ) goto transition_cancle;



            if (tok.content == "//") is_comment = true;

            if (
                state_old != CharType::Invalid &&  
                state_old != CharType::Format  &&  
                (!is_comment || is_string)
            )
            {
                tok.type = classify(tok.content, state_old);
                tok.line = line;

                out.content.push_back(tok);

                tok = Token();
            }
            else
                tok.content.clear();
        }
        transition_cancle:

        if (state_new == CharType::Quote)
            is_string = !is_string;

        if (c == '\n')
        {
            line++;
            is_comment = false;
        }
        
        tok.content += c;
        state_old = state_new;
    }
    
    return out;
}





}







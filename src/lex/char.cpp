#include <gearlang/lex.hpp>

Lexer::CharType Lexer::getCharType(char c) {
    if (isalpha(c)) return CharType::Alpha;
    if (isdigit(c)) return CharType::Num;

    switch(c) {
        case('_'): return CharType::Alpha;
        case('.'): return CharType::Num;
        case('('): // Next
        case(')'): return CharType::Paren;
        case('{'): // Next
        case('}'): return CharType::Brace;
        case(' '):  // Next
        case('\n'): // Next
        case('\t'): return CharType::Format;
        case('"'): return CharType::Quote;
        case('^'): return CharType::Caret;
        case(';'): return CharType::Semi;
        case('#'): return CharType::Hash;
        case('@'): return CharType::At;
        default: return CharType::Sym;
    }
}

bool is_single_char_token(Lexer::CharType t) {
    using Lexer::CharType;
    
    switch(t) {
    case(CharType::Paren):
    case(CharType::Brace):
    case(CharType::Caret):
    case(CharType::Semi):
    case(CharType::Hash):
    case(CharType::At):
        return true;
    default: return false;
    }
}
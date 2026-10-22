#ifndef TOKEN_H
#define TOKEN_H

#include <string>

enum class TokenType {
    KEYWORD_SELECT,
    KEYWORD_FROM,
    KEYWORD_WHERE,
    KEYWORD_AND,
    KEYWORD_OR,

    IDENTIFIER,
    INTEGER_LITERAL,
    FLOAT_LITERAL,
    STRING_LITERAL,

    EQUAL,            // =
    NOT_EQUAL,        // <> o !=
    LESS_THAN,        // <
    GREATER_THAN,     // >
    LESS_EQUAL,       // <=
    GREATER_EQUAL,    // >=

    COMMA,            // ,
    SEMICOLON,        // ;

    TOKEN_EOF,        // Fin de entrada
    TOKEN_ERROR       // Error léxico
};

struct Token {
    TokenType type;
    std::string lexeme;
    int line;
    int column;
};

std::string tokenTypeToString(TokenType type);

#endif // TOKEN_H

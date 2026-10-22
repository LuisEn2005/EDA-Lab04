#ifndef LEXER_H
#define LEXER_H

#include "Token.h"
#include <string>
#include <vector>
#include <unordered_map>

class Lexer {
private:
    std::string src;
    size_t pos = 0;
    int line = 1;
    int column = 1;

    std::unordered_map<std::string, TokenType> keywords;

    char peek() const;
    char advance();
    void skipWhitespace();
    std::string toUpper(const std::string& str) const;

public:
    explicit Lexer(std::string source);
    Token nextToken();
    std::vector<Token> tokenizeAll();
};

#endif // LEXER_H

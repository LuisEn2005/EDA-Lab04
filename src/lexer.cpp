#include "Lexer.h"
#include <cctype>
#include <utility>

std::string tokenTypeToString(TokenType type) {
    switch (type) {
        case TokenType::KEYWORD_SELECT: return "KEYWORD_SELECT";
        case TokenType::KEYWORD_FROM:   return "KEYWORD_FROM";
        case TokenType::KEYWORD_WHERE:  return "KEYWORD_WHERE";
        case TokenType::KEYWORD_AND:    return "KEYWORD_AND";
        case TokenType::KEYWORD_OR:     return "KEYWORD_OR";
        case TokenType::IDENTIFIER:      return "IDENTIFIER";
        case TokenType::INTEGER_LITERAL: return "INTEGER_LITERAL";
        case TokenType::FLOAT_LITERAL:   return "FLOAT_LITERAL";
        case TokenType::STRING_LITERAL:  return "STRING_LITERAL";
        case TokenType::EQUAL:          return "EQUAL";
        case TokenType::NOT_EQUAL:      return "NOT_EQUAL";
        case TokenType::LESS_THAN:      return "LESS_THAN";
        case TokenType::GREATER_THAN:   return "GREATER_THAN";
        case TokenType::LESS_EQUAL:     return "LESS_EQUAL";
        case TokenType::GREATER_EQUAL:  return "GREATER_EQUAL";
        case TokenType::COMMA:          return "COMMA";
        case TokenType::SEMICOLON:      return "SEMICOLON";
        case TokenType::TOKEN_EOF:      return "EOF";
        case TokenType::TOKEN_ERROR:    return "ERROR";
        default:                        return "UNKNOWN";
    }
}

Lexer::Lexer(std::string source) : src(std::move(source)) {
    keywords = {
        {"SELECT", TokenType::KEYWORD_SELECT},
        {"FROM",   TokenType::KEYWORD_FROM},
        {"WHERE",  TokenType::KEYWORD_WHERE},
        {"AND",    TokenType::KEYWORD_AND},
        {"OR",     TokenType::KEYWORD_OR}
    };
}

char Lexer::peek() const {
    if (pos >= src.length()) return '\0';
    return src[pos];
}

char Lexer::advance() {
    char current = peek();
    pos++;
    if (current == '\n') {
        line++;
        column = 1;
    } else {
        column++;
    }
    return current;
}

void Lexer::skipWhitespace() {
    while (pos < src.length()) {
        char c = peek();
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            advance();
        } else {
            break;
        }
    }
}

std::string Lexer::toUpper(const std::string& str) const {
    std::string upperStr = str;
    for (char& c : upperStr) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    return upperStr;
}

Token Lexer::nextToken() {
    skipWhitespace();

    int startLine = line;
    int startColumn = column;

    if (pos >= src.length()) {
        return {TokenType::TOKEN_EOF, "EOF", startLine, startColumn};
    }

    char c = peek();

    if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
        std::string lexeme;
        while (pos < src.length() && (std::isalnum(static_cast<unsigned char>(peek())) || peek() == '_')) {
            lexeme += advance();
        }

        std::string upperLexeme = toUpper(lexeme);
        auto it = keywords.find(upperLexeme);
        if (it != keywords.end()) {
            return {it->second, lexeme, startLine, startColumn};
        }
        return {TokenType::IDENTIFIER, lexeme, startLine, startColumn};
    }

    if (std::isdigit(static_cast<unsigned char>(c))) {
        std::string lexeme;
        bool isFloat = false;

        while (pos < src.length() && std::isdigit(static_cast<unsigned char>(peek()))) {
            lexeme += advance();
        }

        if (peek() == '.' && pos + 1 < src.length() && std::isdigit(static_cast<unsigned char>(src[pos + 1]))) {
            isFloat = true;
            lexeme += advance(); // Consumir '.'
            while (pos < src.length() && std::isdigit(static_cast<unsigned char>(peek()))) {
                lexeme += advance();
            }
        }

        return {isFloat ? TokenType::FLOAT_LITERAL : TokenType::INTEGER_LITERAL, lexeme, startLine, startColumn};
    }

    if (c == ''') {
        advance();
        std::string lexeme;
        while (pos < src.length() && peek() != ''') {
            if (peek() == '\n') {
                break;
            }
            lexeme += advance();
        }

        if (peek() == ''') {
            advance();
            return {TokenType::STRING_LITERAL, lexeme, startLine, startColumn};
        } else {
            return {TokenType::TOKEN_ERROR, "Error: Cadena de texto no cerrada", startLine, startColumn};
        }
    }

    switch (c) {
        case ',': advance(); return {TokenType::COMMA, ",", startLine, startColumn};
        case ';': advance(); return {TokenType::SEMICOLON, ";", startLine, startColumn};
        case '=': advance(); return {TokenType::EQUAL, "=", startLine, startColumn};
        case '>':
            advance();
            if (peek() == '=') {
                advance();
                return {TokenType::GREATER_EQUAL, ">=", startLine, startColumn};
            }
            return {TokenType::GREATER_THAN, ">", startLine, startColumn};
        case '<':
            advance();
            if (peek() == '=') {
                advance();
                return {TokenType::LESS_EQUAL, "<=", startLine, startColumn};
            } else if (peek() == '>') {
                advance();
                return {TokenType::NOT_EQUAL, "<>", startLine, startColumn};
            }
            return {TokenType::LESS_THAN, "<", startLine, startColumn};
        case '!':
            advance();
            if (peek() == '=') {
                advance();
                return {TokenType::NOT_EQUAL, "!=", startLine, startColumn};
            }
            return {TokenType::TOKEN_ERROR, "Error: Caracter no valido '!'", startLine, startColumn};
        default:
            char invalidChar = advance();
            return {TokenType::TOKEN_ERROR, std::string("Error: Caracter no valido '") + invalidChar + "'", startLine, startColumn};
    }
}

std::vector<Token> Lexer::tokenizeAll() {
    std::vector<Token> tokens;
    Token t;
    do {
        t = nextToken();
        tokens.push_back(t);
    } while (t.type != TokenType::TOKEN_EOF);
    return tokens;
}

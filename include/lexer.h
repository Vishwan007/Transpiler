#ifndef LEXER_H
#define LEXER_H

#include "token.h"
#include "error_handler.h"
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

class Lexer {
private:
    std::string source;
    int position;
    int line;
    int column;
    int tokenStartColumn;
    std::vector<Token> tokens;
    std::shared_ptr<ErrorHandler> errorHandler;
    
    // Keyword map for efficient lookup
    static const std::unordered_map<std::string, TokenType> keywords;
    
    char currentChar();
    char peekChar(int offset = 1);
    void advance();
    void skipWhitespace();
    void skipComment();
    
    Token readNumber();
    Token readString();
    Token readIdentifierOrKeyword();
    Token readOperator();
    
    bool isDigit(char c) const;
    bool isAlpha(char c) const;
    bool isAlphaNumeric(char c) const;
    bool isOperatorChar(char c) const;
    
public:
    Lexer(const std::string& src, std::shared_ptr<ErrorHandler> errHandler);
    
    std::vector<Token> tokenize();
    Token nextToken();
    
    // Utility
    void reset();
    int getCurrentLine() const { return line; }
    int getCurrentColumn() const { return column; }
};

#endif // LEXER_H

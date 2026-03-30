#ifndef TOKEN_H
#define TOKEN_H

#include <string>

enum class TokenType {
    // Literals
    NUMBER,
    STRING,
    IDENTIFIER,
    
    // Keywords
    SELECT, FROM, WHERE, GROUP, BY, ORDER, HAVING, LIMIT,
    INSERT, INTO, VALUES, UPDATE, SET, DELETE,
    CREATE, TABLE, VARCHAR, INT, TEXT, DATE, BOOLEAN,
    JOIN, INNER, LEFT, RIGHT, FULL, OUTER, ON,
    AND, OR, NOT, IN, LIKE, IS, NULL_KW, BETWEEN,
    COUNT, SUM, AVG, MIN, MAX,
    BEGIN, COMMIT, ROLLBACK, TRANSACTION,
    AS, DISTINCT, CASE, WHEN, THEN, ELSE, END,
    TRUE, FALSE,
    
    // Operators
    PLUS, MINUS, MULTIPLY, DIVIDE, MODULO,
    EQUAL, NOT_EQUAL, LESS_THAN, GREATER_THAN,
    LESS_EQUAL, GREATER_EQUAL, ASSIGN,
    LIKE_OP,
    
    // Delimiters
    LPAREN, RPAREN,
    COMMA, SEMICOLON, DOT,
    
    // Special
    EOF_TOKEN,
    UNKNOWN
};

struct Token {
    TokenType type;
    std::string value;
    int line;
    int column;
    
    Token() : type(TokenType::UNKNOWN), value(""), line(0), column(0) {}
    Token(TokenType t, const std::string& v, int l, int c)
        : type(t), value(v), line(l), column(c) {}
    
    std::string toString() const;
    bool isKeyword() const;
    bool isOperator() const;
    bool isLiteral() const;
};

#endif // TOKEN_H

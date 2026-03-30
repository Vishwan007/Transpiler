#include "lexer.h"
#include <cctype>
#include <algorithm>

const std::unordered_map<std::string, TokenType> Lexer::keywords = {
    {"SELECT", TokenType::SELECT},
    {"FROM", TokenType::FROM},
    {"WHERE", TokenType::WHERE},
    {"GROUP", TokenType::GROUP},
    {"BY", TokenType::BY},
    {"ORDER", TokenType::ORDER},
    {"HAVING", TokenType::HAVING},
    {"LIMIT", TokenType::LIMIT},
    {"INSERT", TokenType::INSERT},
    {"INTO", TokenType::INTO},
    {"VALUES", TokenType::VALUES},
    {"UPDATE", TokenType::UPDATE},
    {"SET", TokenType::SET},
    {"DELETE", TokenType::DELETE},
    {"JOIN", TokenType::JOIN},
    {"INNER", TokenType::INNER},
    {"LEFT", TokenType::LEFT},
    {"RIGHT", TokenType::RIGHT},
    {"FULL", TokenType::FULL},
    {"OUTER", TokenType::OUTER},
    {"ON", TokenType::ON},
    {"AND", TokenType::AND},
    {"OR", TokenType::OR},
    {"NOT", TokenType::NOT},
    {"IN", TokenType::IN},
    {"LIKE", TokenType::LIKE},
    {"IS", TokenType::IS},
    {"NULL", TokenType::NULL_KW},
    {"BETWEEN", TokenType::BETWEEN},
    {"COUNT", TokenType::COUNT},
    {"SUM", TokenType::SUM},
    {"AVG", TokenType::AVG},
    {"MIN", TokenType::MIN},
    {"MAX", TokenType::MAX},
    {"BEGIN", TokenType::BEGIN},
    {"COMMIT", TokenType::COMMIT},
    {"ROLLBACK", TokenType::ROLLBACK},
    {"TRANSACTION", TokenType::TRANSACTION},
    {"AS", TokenType::AS},
    {"DISTINCT", TokenType::DISTINCT},
    {"CASE", TokenType::CASE},
    {"WHEN", TokenType::WHEN},
    {"THEN", TokenType::THEN},
    {"ELSE", TokenType::ELSE},
    {"END", TokenType::END},
    {"TRUE", TokenType::TRUE},
    {"FALSE", TokenType::FALSE},
    {"CREATE", TokenType::CREATE},
    {"TABLE", TokenType::TABLE},
    {"VARCHAR", TokenType::VARCHAR},
    {"INT", TokenType::INT},
    {"TEXT", TokenType::TEXT},
    {"DATE", TokenType::DATE},
    {"BOOLEAN", TokenType::BOOLEAN},
};

Lexer::Lexer(const std::string& src, std::shared_ptr<ErrorHandler> errHandler)
    : source(src), position(0), line(1), column(1), tokenStartColumn(1),
      errorHandler(errHandler) {}

char Lexer::currentChar() {
    if (position >= source.length()) {
        return '\0';
    }
    return source[position];
}

char Lexer::peekChar(int offset) {
    if (position + offset >= source.length()) {
        return '\0';
    }
    return source[position + offset];
}

void Lexer::advance() {
    if (position < source.length()) {
        if (source[position] == '\n') {
            line++;
            column = 1;
        } else {
            column++;
        }
        position++;
    }
}

void Lexer::skipWhitespace() {
    while (currentChar() == ' ' || currentChar() == '\t' ||
           currentChar() == '\n' || currentChar() == '\r') {
        advance();
    }
}

void Lexer::skipComment() {
    if (currentChar() == '-' && peekChar() == '-') {
        while (currentChar() != '\n' && currentChar() != '\0') {
            advance();
        }
        if (currentChar() == '\n') advance();
    } else if (currentChar() == '/' && peekChar() == '*') {
        advance(); advance();
        while (currentChar() != '\0') {
            if (currentChar() == '*' && peekChar() == '/') {
                advance(); advance();
                break;
            }
            advance();
        }
    }
}

bool Lexer::isDigit(char c) const {
    return c >= '0' && c <= '9';
}

bool Lexer::isAlpha(char c) const {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

bool Lexer::isAlphaNumeric(char c) const {
    return isAlpha(c) || isDigit(c);
}

bool Lexer::isOperatorChar(char c) const {
    return c == '+' || c == '-' || c == '*' || c == '/' || c == '%' ||
           c == '=' || c == '<' || c == '>' || c == '!';
}

Token Lexer::readNumber() {
    tokenStartColumn = column;
    std::string value;
    
    while (isDigit(currentChar())) {
        value += currentChar();
        advance();
    }
    
    if (currentChar() == '.' && isDigit(peekChar())) {
        value += currentChar();
        advance();
        while (isDigit(currentChar())) {
            value += currentChar();
            advance();
        }
    }
    
    return Token(TokenType::NUMBER, value, line, tokenStartColumn);
}

Token Lexer::readString() {
    tokenStartColumn = column;
    char quote = currentChar();
    std::string value;
    advance();  // Skip opening quote
    
    while (currentChar() != quote && currentChar() != '\0') {
        if (currentChar() == '\\') {
            advance();
            value += currentChar();
            advance();
        } else {
            value += currentChar();
            advance();
        }
    }
    
    if (currentChar() == quote) {
        advance();  // Skip closing quote
    } else {
        errorHandler->addLexicalError(line, tokenStartColumn, 
                                     "Unterminated string literal", value);
    }
    
    return Token(TokenType::STRING, value, line, tokenStartColumn);
}

Token Lexer::readIdentifierOrKeyword() {
    tokenStartColumn = column;
    std::string value;
    
    while (isAlphaNumeric(currentChar())) {
        value += currentChar();
        advance();
    }
    
    // Convert to uppercase for keyword matching
    std::string upperValue = value;
    std::transform(upperValue.begin(), upperValue.end(), upperValue.begin(), ::toupper);
    
    auto it = keywords.find(upperValue);
    if (it != keywords.end()) {
        return Token(it->second, value, line, tokenStartColumn);
    }
    
    return Token(TokenType::IDENTIFIER, value, line, tokenStartColumn);
}

Token Lexer::readOperator() {
    tokenStartColumn = column;
    std::string value;
    value += currentChar();
    advance();
    
    // Two-character operators
    char next = currentChar();
    if ((value == "=" && (next == '=' || next == '<' || next == '>')) ||
        (value == "<" && (next == '=' || next == '>' || next == '-')) ||
        (value == ">" && next == '=') ||
        (value == "!" && next == '=') ||
        (value == "-" && next == '-')) {
        value += next;
        advance();
    }
    
    TokenType type;
    if (value == "+") type = TokenType::PLUS;
    else if (value == "-") type = TokenType::MINUS;
    else if (value == "*") type = TokenType::MULTIPLY;
    else if (value == "/") type = TokenType::DIVIDE;
    else if (value == "%") type = TokenType::MODULO;
    else if (value == "=") type = TokenType::EQUAL;
    else if (value == "!=" || value == "<>") type = TokenType::NOT_EQUAL;
    else if (value == "<") type = TokenType::LESS_THAN;
    else if (value == ">") type = TokenType::GREATER_THAN;
    else if (value == "<=") type = TokenType::LESS_EQUAL;
    else if (value == ">=") type = TokenType::GREATER_EQUAL;
    else if (value == ":=") type = TokenType::ASSIGN;
    else type = TokenType::UNKNOWN;
    
    return Token(type, value, line, tokenStartColumn);
}

std::vector<Token> Lexer::tokenize() {
    tokens.clear();
    
    while (position < source.length()) {
        skipWhitespace();
        
        if (currentChar() == '\0') break;
        
        // Comments
        if (currentChar() == '-' && peekChar() == '-') {
            skipComment();
            continue;
        }
        if (currentChar() == '/' && peekChar() == '*') {
            skipComment();
            continue;
        }
        
        // Numbers
        if (isDigit(currentChar())) {
            tokens.push_back(readNumber());
        }
        // Strings
        else if (currentChar() == '\'' || currentChar() == '"') {
            tokens.push_back(readString());
        }
        // Identifiers and keywords
        else if (isAlpha(currentChar())) {
            tokens.push_back(readIdentifierOrKeyword());
        }
        // Operators
        else if (isOperatorChar(currentChar())) {
            tokens.push_back(readOperator());
        }
        // Delimiters
        else if (currentChar() == '(') {
            tokens.push_back(Token(TokenType::LPAREN, "(", line, column));
            advance();
        }
        else if (currentChar() == ')') {
            tokens.push_back(Token(TokenType::RPAREN, ")", line, column));
            advance();
        }
        else if (currentChar() == ',') {
            tokens.push_back(Token(TokenType::COMMA, ",", line, column));
            advance();
        }
        else if (currentChar() == ';') {
            tokens.push_back(Token(TokenType::SEMICOLON, ";", line, column));
            advance();
        }
        else if (currentChar() == '.') {
            tokens.push_back(Token(TokenType::DOT, ".", line, column));
            advance();
        }
        else {
            errorHandler->addLexicalError(line, column,
                                         std::string("Unexpected character: '") + currentChar() + "'");
            advance();
        }
    }
    
    tokens.push_back(Token(TokenType::EOF_TOKEN, "", line, column));
    return tokens;
}

Token Lexer::nextToken() {
    if (position >= tokens.size()) {
        return Token(TokenType::EOF_TOKEN, "", line, column);
    }
    return tokens[position++];
}

void Lexer::reset() {
    position = 0;
    line = 1;
    column = 1;
}

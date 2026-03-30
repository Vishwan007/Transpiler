#include "token.h"

std::string Token::toString() const {
    std::string typeStr;
    switch (type) {
        case TokenType::NUMBER:     typeStr = "NUMBER"; break;
        case TokenType::STRING:     typeStr = "STRING"; break;
        case TokenType::IDENTIFIER: typeStr = "IDENTIFIER"; break;
        case TokenType::SELECT:     typeStr = "SELECT"; break;
        case TokenType::FROM:       typeStr = "FROM"; break;
        case TokenType::WHERE:      typeStr = "WHERE"; break;
        case TokenType::GROUP:      typeStr = "GROUP"; break;
        case TokenType::BY:         typeStr = "BY"; break;
        case TokenType::ORDER:      typeStr = "ORDER"; break;
        case TokenType::INSERT:     typeStr = "INSERT"; break;
        case TokenType::UPDATE:     typeStr = "UPDATE"; break;
        case TokenType::DELETE:     typeStr = "DELETE"; break;
        case TokenType::JOIN:       typeStr = "JOIN"; break;
        case TokenType::AND:        typeStr = "AND"; break;
        case TokenType::OR:         typeStr = "OR"; break;
        case TokenType::NOT:        typeStr = "NOT"; break;
        case TokenType::EQUAL:      typeStr = "EQUAL"; break;
        case TokenType::NOT_EQUAL:  typeStr = "NOT_EQUAL"; break;
        case TokenType::LPAREN:     typeStr = "LPAREN"; break;
        case TokenType::RPAREN:     typeStr = "RPAREN"; break;
        case TokenType::COMMA:      typeStr = "COMMA"; break;
        case TokenType::SEMICOLON:  typeStr = "SEMICOLON"; break;
        case TokenType::EOF_TOKEN:  typeStr = "EOF"; break;
        default:                    typeStr = "UNKNOWN"; break;
    }
    
    return typeStr + "(" + value + ")";
}

bool Token::isKeyword() const {
    return type >= TokenType::SELECT && type <= TokenType::FALSE;
}

bool Token::isOperator() const {
    return (type >= TokenType::PLUS && type <= TokenType::ASSIGN) ||
           type == TokenType::LIKE_OP;
}

bool Token::isLiteral() const {
    return type == TokenType::NUMBER || type == TokenType::STRING ||
           type == TokenType::TRUE || type == TokenType::FALSE ||
           type == TokenType::NULL_KW;
}

#ifndef PARSER_H
#define PARSER_H

#include "token.h"
#include "ast.h"
#include "error_handler.h"
#include <vector>
#include <memory>

class Parser {
private:
    std::vector<Token> tokens;
    int position;
    std::shared_ptr<ErrorHandler> errorHandler;
    
    Token currentToken();
    Token peekToken(int offset = 1);
    void advance();
    bool match(TokenType type);
    bool check(TokenType type);
    Token consume(TokenType type, const std::string& message);
    
    // Helper functions
    bool isAtEnd() const;
    void synchronize();
    
    // Parsing functions - Statement level
    std::shared_ptr<ASTNode> parseStatement();
    std::shared_ptr<SelectStatement> parseSelectStatement();
    std::shared_ptr<InsertStatement> parseInsertStatement();
    std::shared_ptr<UpdateStatement> parseUpdateStatement();
    std::shared_ptr<DeleteStatement> parseDeleteStatement();
    std::shared_ptr<TransactionStatement> parseTransactionStatement();
    
    // SELECT clause components
    std::shared_ptr<SelectList> parseSelectList();
    std::shared_ptr<TableReference> parseFromClause();
    std::shared_ptr<TableReference> parseJoinClause(std::shared_ptr<TableReference> left);
    std::shared_ptr<WhereClause> parseWhereClause();
    std::shared_ptr<GroupByClause> parseGroupByClause();
    std::shared_ptr<OrderByClause> parseOrderByClause();
    
    // Expression parsing (with operator precedence)
    std::shared_ptr<Expression> parseExpression();
    std::shared_ptr<Expression> parseLogicalOr();
    std::shared_ptr<Expression> parseLogicalAnd();
    std::shared_ptr<Expression> parseComparison();
    std::shared_ptr<Expression> parseAdditive();
    std::shared_ptr<Expression> parseMultiplicative();
    std::shared_ptr<Expression> parseUnary();
    std::shared_ptr<Expression> parsePrimary();
    std::shared_ptr<Expression> parseFunction();
    std::shared_ptr<Expression> parseCaseExpression();
    
public:
    Parser(const std::vector<Token>& tokenList, std::shared_ptr<ErrorHandler> errHandler);
    
    std::shared_ptr<ASTNode> parse();
    
    // Error handling
    bool hasErrors() const { return errorHandler->hasErrors(); }
    bool hasCriticalErrors() const { return errorHandler->hasCriticalErrors(); }
};

#endif // PARSER_H

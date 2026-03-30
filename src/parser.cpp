#include "parser.h"
#include <sstream>

Parser::Parser(const std::vector<Token>& tokenList, std::shared_ptr<ErrorHandler> errHandler)
    : tokens(tokenList), position(0), errorHandler(errHandler) {}

Token Parser::currentToken() {
    if (position >= tokens.size()) {
        return Token(TokenType::EOF_TOKEN, "", 0, 0);
    }
    return tokens[position];
}

Token Parser::peekToken(int offset) {
    if (position + offset >= tokens.size()) {
        return Token(TokenType::EOF_TOKEN, "", 0, 0);
    }
    return tokens[position + offset];
}

void Parser::advance() {
    if (position < tokens.size()) {
        position++;
    }
}

bool Parser::match(TokenType type) {
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}

bool Parser::check(TokenType type) {
    if (isAtEnd()) return false;
    return currentToken().type == type;
}

Token Parser::consume(TokenType type, const std::string& message) {
    Token token = currentToken();
    if (token.type == type) {
        advance();
        return token;
    }
    
    errorHandler->addSyntaxError(token.line, token.column, message, token.value);
    return Token(type, "", token.line, token.column);
}

bool Parser::isAtEnd() const {
    return position >= tokens.size() || currentToken().type == TokenType::EOF_TOKEN;
}

void Parser::synchronize() {
    advance();
    while (!isAtEnd()) {
        if (tokens[position - 1].type == TokenType::SEMICOLON) return;
        
        switch (currentToken().type) {
            case TokenType::SELECT:
            case TokenType::INSERT:
            case TokenType::UPDATE:
            case TokenType::DELETE:
            case TokenType::BEGIN:
                return;
            default:
                advance();
        }
    }
}

std::shared_ptr<ASTNode> Parser::parse() {
    return parseStatement();
}

std::shared_ptr<ASTNode> Parser::parseStatement() {
    try {
        if (match(TokenType::SELECT)) {
            position--; // Back up
            return parseSelectStatement();
        }
        else if (match(TokenType::INSERT)) {
            position--; // Back up
            return parseInsertStatement();
        }
        else if (match(TokenType::UPDATE)) {
            position--; // Back up
            return parseUpdateStatement();
        }
        else if (match(TokenType::DELETE)) {
            position--; // Back up
            return parseDeleteStatement();
        }
        else if (match(TokenType::BEGIN) || match(TokenType::COMMIT) || match(TokenType::ROLLBACK)) {
            position--; // Back up
            return parseTransactionStatement();
        }
        else {
            Token token = currentToken();
            errorHandler->addSyntaxError(token.line, token.column,
                                        "Expected SELECT, INSERT, UPDATE, DELETE, or transaction statement",
                                        token.value);
            synchronize();
            return nullptr;
        }
    } catch (...) {
        synchronize();
        return nullptr;
    }
}

std::shared_ptr<SelectStatement> Parser::parseSelectStatement() {
    auto stmt = std::make_shared<SelectStatement>();
    
    if (!match(TokenType::SELECT)) {
        errorHandler->addSyntaxError(currentToken().line, currentToken().column,
                                    "Expected SELECT keyword");
        return stmt;
    }
    
    // DISTINCT modifier
    bool isDistinct = match(TokenType::DISTINCT);
    
    // Parse select list
    stmt->selectList = parseSelectList();
    if (!stmt->selectList) {
        errorHandler->addSyntaxError(currentToken().line, currentToken().column,
                                    "Invalid SELECT list");
        return stmt;
    }
    stmt->selectList->isDistinct = isDistinct;
    
    // FROM clause
    if (match(TokenType::FROM)) {
        stmt->fromClause = parseFromClause();
        if (!stmt->fromClause) {
            errorHandler->addSyntaxError(currentToken().line, currentToken().column,
                                        "Invalid FROM clause");
            return stmt;
        }
    }
    
    // WHERE clause
    if (check(TokenType::WHERE)) {
        stmt->whereClause = parseWhereClause();
    }
    
    // GROUP BY clause
    if (check(TokenType::GROUP)) {
        stmt->groupByClause = parseGroupByClause();
    }
    
    // ORDER BY clause
    if (check(TokenType::ORDER)) {
        stmt->orderByClause = parseOrderByClause();
    }
    
    // LIMIT clause
    if (match(TokenType::LIMIT)) {
        if (check(TokenType::NUMBER)) {
            stmt->limitValue = std::stoi(currentToken().value);
            advance();
        } else {
            errorHandler->addSyntaxError(currentToken().line, currentToken().column,
                                        "Expected number after LIMIT");
        }
    }
    
    return stmt;
}

std::shared_ptr<SelectList> Parser::parseSelectList() {
    auto selectList = std::make_shared<SelectList>();
    
    do {
        auto expr = parseExpression();
        if (!expr) {
            errorHandler->addSyntaxError(currentToken().line, currentToken().column,
                                        "Invalid expression in SELECT list");
            return nullptr;
        }
        
        // Handle AS alias
        if (match(TokenType::AS)) {
            if (check(TokenType::IDENTIFIER)) {
                if (auto id = std::dynamic_pointer_cast<Identifier>(expr)) {
                    id->alias = currentToken().value;
                }
                advance();
            }
        }
        
        selectList->expressions.push_back(expr);
    } while (match(TokenType::COMMA));
    
    return selectList;
}

std::shared_ptr<TableReference> Parser::parseFromClause() {
    if (!check(TokenType::IDENTIFIER)) {
        errorHandler->addSyntaxError(currentToken().line, currentToken().column,
                                    "Expected table name");
        return nullptr;
    }
    
    auto tableRef = std::make_shared<TableReference>(currentToken().value);
    advance();
    
    // Handle table alias
    if (match(TokenType::AS)) {
        if (check(TokenType::IDENTIFIER)) {
            tableRef->alias = currentToken().value;
            advance();
        }
    } else if (check(TokenType::IDENTIFIER) && 
               currentToken().type != TokenType::WHERE &&
               currentToken().type != TokenType::GROUP &&
               currentToken().type != TokenType::ORDER &&
               currentToken().type != TokenType::LIMIT) {
        tableRef->alias = currentToken().value;
        advance();
    }
    
    // Handle JOINs
    while (check(TokenType::JOIN) || check(TokenType::INNER) || 
           check(TokenType::LEFT) || check(TokenType::RIGHT)) {
        tableRef = parseJoinClause(tableRef);
        if (!tableRef) return nullptr;
    }
    
    return tableRef;
}

std::shared_ptr<TableReference> Parser::parseJoinClause(std::shared_ptr<TableReference> left) {
    std::string joinType = "INNER";
    
    if (match(TokenType::LEFT)) {
        joinType = "LEFT";
        match(TokenType::OUTER); // Optional OUTER
    } else if (match(TokenType::RIGHT)) {
        joinType = "RIGHT";
        match(TokenType::OUTER); // Optional OUTER
    } else if (match(TokenType::FULL)) {
        joinType = "FULL";
        match(TokenType::OUTER); // Optional OUTER
    } else if (match(TokenType::INNER)) {
        joinType = "INNER";
    }
    
    if (!match(TokenType::JOIN)) {
        errorHandler->addSyntaxError(currentToken().line, currentToken().column,
                                    "Expected JOIN keyword");
        return nullptr;
    }
    
    if (!check(TokenType::IDENTIFIER)) {
        errorHandler->addSyntaxError(currentToken().line, currentToken().column,
                                    "Expected table name after JOIN");
        return nullptr;
    }
    
    auto rightTable = std::make_shared<TableReference>(currentToken().value);
    advance();
    
    // Handle alias
    if (match(TokenType::AS)) {
        if (check(TokenType::IDENTIFIER)) {
            rightTable->alias = currentToken().value;
            advance();
        }
    }
    
    rightTable->joinType = joinType;
    
    // ON condition
    if (match(TokenType::ON)) {
        rightTable->joinCondition = parseExpression();
        if (!rightTable->joinCondition) {
            errorHandler->addSyntaxError(currentToken().line, currentToken().column,
                                        "Invalid JOIN condition");
            return nullptr;
        }
    } else {
        errorHandler->addSyntaxError(currentToken().line, currentToken().column,
                                    "Expected ON clause after JOIN");
    }
    
    left->joins.push_back(rightTable);
    return left;
}

std::shared_ptr<WhereClause> Parser::parseWhereClause() {
    if (!match(TokenType::WHERE)) {
        return nullptr;
    }
    
    auto condition = parseExpression();
    if (!condition) {
        errorHandler->addSyntaxError(currentToken().line, currentToken().column,
                                    "Invalid WHERE condition");
        return nullptr;
    }
    
    return std::make_shared<WhereClause>(condition);
}

std::shared_ptr<GroupByClause> Parser::parseGroupByClause() {
    if (!match(TokenType::GROUP)) {
        return nullptr;
    }
    
    if (!match(TokenType::BY)) {
        errorHandler->addSyntaxError(currentToken().line, currentToken().column,
                                    "Expected BY after GROUP");
        return nullptr;
    }
    
    auto groupBy = std::make_shared<GroupByClause>();
    
    do {
        auto expr = parseExpression();
        if (!expr) {
            errorHandler->addSyntaxError(currentToken().line, currentToken().column,
                                        "Invalid expression in GROUP BY");
            return nullptr;
        }
        groupBy->groupExpressions.push_back(expr);
    } while (match(TokenType::COMMA));
    
    // HAVING clause
    if (match(TokenType::HAVING)) {
        groupBy->havingCondition = parseExpression();
        if (!groupBy->havingCondition) {
            errorHandler->addSyntaxError(currentToken().line, currentToken().column,
                                        "Invalid HAVING condition");
            return nullptr;
        }
    }
    
    return groupBy;
}

std::shared_ptr<OrderByClause> Parser::parseOrderByClause() {
    if (!match(TokenType::ORDER)) {
        return nullptr;
    }
    
    if (!match(TokenType::BY)) {
        errorHandler->addSyntaxError(currentToken().line, currentToken().column,
                                    "Expected BY after ORDER");
        return nullptr;
    }
    
    auto orderBy = std::make_shared<OrderByClause>();
    
    do {
        auto expr = parseExpression();
        if (!expr) {
            errorHandler->addSyntaxError(currentToken().line, currentToken().column,
                                        "Invalid expression in ORDER BY");
            return nullptr;
        }
        
        // ASC or DESC
        bool isAsc = true;
        if (currentToken().value == "DESC" || currentToken().value == "desc") {
            isAsc = false;
            advance();
        } else if (currentToken().value == "ASC" || currentToken().value == "asc") {
            isAsc = true;
            advance();
        }
        
        orderBy->orderItems.emplace_back(expr, isAsc);
    } while (match(TokenType::COMMA));
    
    return orderBy;
}

std::shared_ptr<InsertStatement> Parser::parseInsertStatement() {
    auto stmt = std::make_shared<InsertStatement>();
    
    if (!match(TokenType::INSERT)) {
        errorHandler->addSyntaxError(currentToken().line, currentToken().column,
                                    "Expected INSERT keyword");
        return stmt;
    }
    
    if (!match(TokenType::INTO)) {
        errorHandler->addSyntaxError(currentToken().line, currentToken().column,
                                    "Expected INTO after INSERT");
        return stmt;
    }
    
    if (!check(TokenType::IDENTIFIER)) {
        errorHandler->addSyntaxError(currentToken().line, currentToken().column,
                                    "Expected table name");
        return stmt;
    }
    
    stmt->tableName = currentToken().value;
    advance();
    
    // Column names
    if (match(TokenType::LPAREN)) {
        do {
            if (!check(TokenType::IDENTIFIER)) {
                errorHandler->addSyntaxError(currentToken().line, currentToken().column,
                                            "Expected column name");
                return stmt;
            }
            stmt->columnNames.push_back(currentToken().value);
            advance();
        } while (match(TokenType::COMMA));
        
        if (!match(TokenType::RPAREN)) {
            errorHandler->addSyntaxError(currentToken().line, currentToken().column,
                                        "Expected ) after column list");
            return stmt;
        }
    }
    
    // VALUES
    if (!match(TokenType::VALUES)) {
        errorHandler->addSyntaxError(currentToken().line, currentToken().column,
                                    "Expected VALUES keyword");
        return stmt;
    }
    
    do {
        if (!match(TokenType::LPAREN)) {
            errorHandler->addSyntaxError(currentToken().line, currentToken().column,
                                        "Expected ( before values");
            return stmt;
        }
        
        std::vector<std::shared_ptr<Expression>> values;
        do {
            auto expr = parseExpression();
            if (!expr) {
                errorHandler->addSyntaxError(currentToken().line, currentToken().column,
                                            "Invalid value expression");
                return stmt;
            }
            values.push_back(expr);
        } while (match(TokenType::COMMA));
        
        if (!match(TokenType::RPAREN)) {
            errorHandler->addSyntaxError(currentToken().line, currentToken().column,
                                        "Expected ) after values");
            return stmt;
        }
        
        stmt->valuesList.push_back(values);
    } while (match(TokenType::COMMA));
    
    return stmt;
}

std::shared_ptr<UpdateStatement> Parser::parseUpdateStatement() {
    auto stmt = std::make_shared<UpdateStatement>();
    
    if (!match(TokenType::UPDATE)) {
        errorHandler->addSyntaxError(currentToken().line, currentToken().column,
                                    "Expected UPDATE keyword");
        return stmt;
    }
    
    if (!check(TokenType::IDENTIFIER)) {
        errorHandler->addSyntaxError(currentToken().line, currentToken().column,
                                    "Expected table name");
        return stmt;
    }
    
    stmt->tableName = currentToken().value;
    advance();
    
    if (!match(TokenType::SET)) {
        errorHandler->addSyntaxError(currentToken().line, currentToken().column,
                                    "Expected SET keyword");
        return stmt;
    }
    
    do {
        if (!check(TokenType::IDENTIFIER)) {
            errorHandler->addSyntaxError(currentToken().line, currentToken().column,
                                        "Expected column name");
            return stmt;
        }
        
        std::string columnName = currentToken().value;
        advance();
        
        if (!match(TokenType::EQUAL)) {
            errorHandler->addSyntaxError(currentToken().line, currentToken().column,
                                        "Expected = after column name");
            return stmt;
        }
        
        auto value = parseExpression();
        if (!value) {
            errorHandler->addSyntaxError(currentToken().line, currentToken().column,
                                        "Invalid value expression");
            return stmt;
        }
        
        stmt->assignments.emplace_back(columnName, value);
    } while (match(TokenType::COMMA));
    
    if (match(TokenType::WHERE)) {
        stmt->whereCondition = parseExpression();
        if (!stmt->whereCondition) {
            errorHandler->addSyntaxError(currentToken().line, currentToken().column,
                                        "Invalid WHERE condition");
            return stmt;
        }
    }
    
    return stmt;
}

std::shared_ptr<DeleteStatement> Parser::parseDeleteStatement() {
    auto stmt = std::make_shared<DeleteStatement>();
    
    if (!match(TokenType::DELETE)) {
        errorHandler->addSyntaxError(currentToken().line, currentToken().column,
                                    "Expected DELETE keyword");
        return stmt;
    }
    
    if (!match(TokenType::FROM)) {
        errorHandler->addSyntaxError(currentToken().line, currentToken().column,
                                    "Expected FROM after DELETE");
        return stmt;
    }
    
    if (!check(TokenType::IDENTIFIER)) {
        errorHandler->addSyntaxError(currentToken().line, currentToken().column,
                                    "Expected table name");
        return stmt;
    }
    
    stmt->tableName = currentToken().value;
    advance();
    
    if (match(TokenType::WHERE)) {
        stmt->whereCondition = parseExpression();
        if (!stmt->whereCondition) {
            errorHandler->addSyntaxError(currentToken().line, currentToken().column,
                                        "Invalid WHERE condition");
            return stmt;
        }
    }
    
    return stmt;
}

std::shared_ptr<TransactionStatement> Parser::parseTransactionStatement() {
    if (match(TokenType::BEGIN)) {
        match(TokenType::TRANSACTION); // Optional TRANSACTION keyword
        return std::make_shared<TransactionStatement>(TransactionStatement::Type::BEGIN);
    } else if (match(TokenType::COMMIT)) {
        return std::make_shared<TransactionStatement>(TransactionStatement::Type::COMMIT);
    } else if (match(TokenType::ROLLBACK)) {
        return std::make_shared<TransactionStatement>(TransactionStatement::Type::ROLLBACK);
    }
    
    errorHandler->addSyntaxError(currentToken().line, currentToken().column,
                                "Expected transaction statement");
    return nullptr;
}

std::shared_ptr<Expression> Parser::parseExpression() {
    return parseLogicalOr();
}

std::shared_ptr<Expression> Parser::parseLogicalOr() {
    auto expr = parseLogicalAnd();
    
    while (match(TokenType::OR)) {
        auto op = tokens[position - 1].value;
        auto right = parseLogicalAnd();
        if (!right) {
            errorHandler->addSyntaxError(currentToken().line, currentToken().column,
                                        "Invalid right operand for OR");
            return expr;
        }
        expr = std::make_shared<BinaryOp>(expr, op, right);
    }
    
    return expr;
}

std::shared_ptr<Expression> Parser::parseLogicalAnd() {
    auto expr = parseComparison();
    
    while (match(TokenType::AND)) {
        auto op = tokens[position - 1].value;
        auto right = parseComparison();
        if (!right) {
            errorHandler->addSyntaxError(currentToken().line, currentToken().column,
                                        "Invalid right operand for AND");
            return expr;
        }
        expr = std::make_shared<BinaryOp>(expr, op, right);
    }
    
    return expr;
}

std::shared_ptr<Expression> Parser::parseComparison() {
    auto expr = parseAdditive();
    
    if (match(TokenType::EQUAL) || match(TokenType::NOT_EQUAL) ||
        match(TokenType::LESS_THAN) || match(TokenType::GREATER_THAN) ||
        match(TokenType::LESS_EQUAL) || match(TokenType::GREATER_EQUAL)) {
        
        auto op = tokens[position - 1].value;
        auto right = parseAdditive();
        if (!right) {
            errorHandler->addSyntaxError(currentToken().line, currentToken().column,
                                        "Invalid right operand");
            return expr;
        }
        expr = std::make_shared<BinaryOp>(expr, op, right);
    }
    else if (match(TokenType::LIKE)) {
        auto right = parseAdditive();
        if (!right) {
            errorHandler->addSyntaxError(currentToken().line, currentToken().column,
                                        "Invalid right operand for LIKE");
            return expr;
        }
        expr = std::make_shared<BinaryOp>(expr, "LIKE", right);
    }
    else if (match(TokenType::IN)) {
        if (match(TokenType::LPAREN)) {
            auto right = parseExpression();
            if (!match(TokenType::RPAREN)) {
                errorHandler->addSyntaxError(currentToken().line, currentToken().column,
                                            "Expected ) in IN clause");
                return expr;
            }
            expr = std::make_shared<BinaryOp>(expr, "IN", right);
        }
    }
    else if (match(TokenType::BETWEEN)) {
        auto lower = parseAdditive();
        if (!match(TokenType::AND)) {
            errorHandler->addSyntaxError(currentToken().line, currentToken().column,
                                        "Expected AND in BETWEEN clause");
            return expr;
        }
        auto upper = parseAdditive();
        // Create nested expression for BETWEEN
        expr = std::make_shared<BinaryOp>(expr, "BETWEEN", lower);
    }
    else if (match(TokenType::IS)) {
        bool isNull = match(TokenType::NULL_KW);
        if (isNull) {
            expr = std::make_shared<BinaryOp>(expr, "IS", std::make_shared<Literal>("NULL", Literal::Type::NULL_VAL));
        }
    }
    
    return expr;
}

std::shared_ptr<Expression> Parser::parseAdditive() {
    auto expr = parseMultiplicative();
    
    while (match(TokenType::PLUS) || match(TokenType::MINUS)) {
        auto op = tokens[position - 1].value;
        auto right = parseMultiplicative();
        if (!right) {
            errorHandler->addSyntaxError(currentToken().line, currentToken().column,
                                        "Invalid right operand");
            return expr;
        }
        expr = std::make_shared<BinaryOp>(expr, op, right);
    }
    
    return expr;
}

std::shared_ptr<Expression> Parser::parseMultiplicative() {
    auto expr = parseUnary();
    
    while (match(TokenType::MULTIPLY) || match(TokenType::DIVIDE) || match(TokenType::MODULO)) {
        auto op = tokens[position - 1].value;
        auto right = parseUnary();
        if (!right) {
            errorHandler->addSyntaxError(currentToken().line, currentToken().column,
                                        "Invalid right operand");
            return expr;
        }
        expr = std::make_shared<BinaryOp>(expr, op, right);
    }
    
    return expr;
}

std::shared_ptr<Expression> Parser::parseUnary() {
    if (match(TokenType::NOT) || match(TokenType::MINUS) || match(TokenType::PLUS)) {
        auto op = tokens[position - 1].value;
        auto expr = parseUnary();
        if (!expr) {
            errorHandler->addSyntaxError(currentToken().line, currentToken().column,
                                        "Invalid unary operand");
            return nullptr;
        }
        return std::make_shared<UnaryOp>(op, expr);
    }
    
    return parsePrimary();
}

std::shared_ptr<Expression> Parser::parsePrimary() {
    if (match(TokenType::LPAREN)) {
        // Could be a subquery or a grouped expression
        if (check(TokenType::SELECT)) {
            // Subquery - for now we'll just skip it
            errorHandler->addSyntaxError(currentToken().line, currentToken().column,
                                        "Subqueries not yet supported");
            return nullptr;
        }
        
        auto expr = parseExpression();
        if (!match(TokenType::RPAREN)) {
            errorHandler->addSyntaxError(currentToken().line, currentToken().column,
                                        "Expected ) after expression");
            return nullptr;
        }
        return expr;
    }
    
    if (match(TokenType::CASE)) {
        return parseCaseExpression();
    }
    
    if (check(TokenType::NUMBER)) {
        auto literal = std::make_shared<Literal>(currentToken().value, Literal::Type::NUMBER);
        advance();
        return literal;
    }
    
    if (check(TokenType::STRING)) {
        auto literal = std::make_shared<Literal>(currentToken().value, Literal::Type::STRING);
        advance();
        return literal;
    }
    
    if (check(TokenType::NULL_KW)) {
        auto literal = std::make_shared<Literal>("NULL", Literal::Type::NULL_VAL);
        advance();
        return literal;
    }
    
    if (check(TokenType::TRUE) || check(TokenType::FALSE)) {
        auto literal = std::make_shared<Literal>(currentToken().value, Literal::Type::BOOLEAN);
        advance();
        return literal;
    }
    
    if (check(TokenType::IDENTIFIER) || check(TokenType::COUNT) || check(TokenType::SUM) ||
        check(TokenType::AVG) || check(TokenType::MIN) || check(TokenType::MAX)) {
        
        // Could be a function call or identifier
        std::string name = currentToken().value;
        Token idToken = currentToken();
        advance();
        
        if (match(TokenType::LPAREN)) {
            // Function call
            auto func = std::make_shared<FunctionCall>(name);
            
            // Check for aggregate functions
            if (name == "COUNT" || name == "SUM" || name == "AVG" || name == "MIN" || name == "MAX") {
                func->isAggregate = true;
            }
            
            if (!check(TokenType::RPAREN)) {
                do {
                    auto arg = parseExpression();
                    if (!arg) {
                        errorHandler->addSyntaxError(currentToken().line, currentToken().column,
                                                    "Invalid function argument");
                        return nullptr;
                    }
                    func->arguments.push_back(arg);
                } while (match(TokenType::COMMA));
            }
            
            if (!match(TokenType::RPAREN)) {
                errorHandler->addSyntaxError(currentToken().line, currentToken().column,
                                            "Expected ) after function arguments");
                return nullptr;
            }
            
            return func;
        } else {
            // Check for table.column notation
            std::string tableAlias;
            if (match(TokenType::DOT)) {
                tableAlias = name;
                if (check(TokenType::IDENTIFIER)) {
                    name = currentToken().value;
                    advance();
                } else {
                    errorHandler->addSyntaxError(currentToken().line, currentToken().column,
                                                "Expected column name after .");
                    return nullptr;
                }
            }
            
            auto id = std::make_shared<Identifier>(name);
            if (!tableAlias.empty()) {
                id->tableAlias = tableAlias;
            }
            return id;
        }
    }
    
    errorHandler->addSyntaxError(currentToken().line, currentToken().column,
                                "Expected expression");
    return nullptr;
}

std::shared_ptr<Expression> Parser::parseCaseExpression() {
    auto caseExpr = std::make_shared<CaseExpression>();
    
    while (match(TokenType::WHEN)) {
        auto condition = parseExpression();
        if (!match(TokenType::THEN)) {
            errorHandler->addSyntaxError(currentToken().line, currentToken().column,
                                        "Expected THEN in CASE expression");
            return nullptr;
        }
        auto result = parseExpression();
        if (!condition || !result) {
            errorHandler->addSyntaxError(currentToken().line, currentToken().column,
                                        "Invalid CASE expression");
            return nullptr;
        }
        caseExpr->whenThen.emplace_back(condition, result);
    }
    
    if (match(TokenType::ELSE)) {
        caseExpr->elseExpr = parseExpression();
    }
    
    if (!match(TokenType::END)) {
        errorHandler->addSyntaxError(currentToken().line, currentToken().column,
                                    "Expected END in CASE expression");
        return nullptr;
    }
    
    return caseExpr;
}

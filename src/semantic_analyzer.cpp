#include "semantic_analyzer.h"
#include <algorithm>

SemanticAnalyzer::SemanticAnalyzer(std::shared_ptr<SymbolTable> symTable, 
                                   std::shared_ptr<ErrorHandler> errHandler)
    : symbolTable(symTable), errorHandler(errHandler) {}

void SemanticAnalyzer::analyze(std::shared_ptr<ASTNode> ast) {
    if (!ast) return;
    ast->accept(this);
}

void SemanticAnalyzer::validateTableExists(const std::string& tableName, int line, int col) {
    if (!symbolTable->isValidTable(tableName)) {
        errorHandler->addSemanticError(line, col,
                                      "Table '" + tableName + "' is not defined", tableName);
    }
}

void SemanticAnalyzer::validateColumnExists(const std::string& tableName, const std::string& columnName, int line, int col) {
    if (!symbolTable->isValidTable(tableName)) {
        errorHandler->addSemanticError(line, col,
                                      "Table '" + tableName + "' is not defined", tableName);
        return;
    }
    
    if (columnName != "*" && !symbolTable->isValidColumn(tableName, columnName)) {
        errorHandler->addSemanticError(line, col,
                                      "Column '" + columnName + "' does not exist in table '" + tableName + "'",
                                      tableName + "." + columnName);
    }
}

void SemanticAnalyzer::analyzeTableReferences(const std::shared_ptr<TableReference>& tableRef) {
    if (!tableRef) return;
    
    validateTableExists(tableRef->tableName, 0, 0);
    
    for (const auto& join : tableRef->joins) {
        validateTableExists(join->tableName, 0, 0);
        if (join->joinCondition) {
            analyzeExpression(join->joinCondition);
        }
    }
}

void SemanticAnalyzer::analyzeExpression(const std::shared_ptr<Expression>& expr) {
    if (!expr) return;
    expr->accept(this);
}

DataType SemanticAnalyzer::inferExpressionType(const std::shared_ptr<Expression>& expr) {
    if (auto lit = std::dynamic_pointer_cast<Literal>(expr)) {
        switch (lit->type) {
            case Literal::Type::NUMBER:
                return (lit->value.find('.') != std::string::npos) ? DataType::FLOAT : DataType::INT;
            case Literal::Type::STRING:
                return DataType::STRING;
            case Literal::Type::BOOLEAN:
                return DataType::BOOLEAN;
            case Literal::Type::NULL_VAL:
                return DataType::UNKNOWN;
        }
    }
    
    if (auto id = std::dynamic_pointer_cast<Identifier>(expr)) {
        // Try to infer from symbol table if available
        return DataType::UNKNOWN;
    }
    
    if (auto binOp = std::dynamic_pointer_cast<BinaryOp>(expr)) {
        auto leftType = inferExpressionType(binOp->left);
        auto rightType = inferExpressionType(binOp->right);
        
        if (binOp->op == "+" || binOp->op == "-" || binOp->op == "*" || binOp->op == "/") {
            if (leftType == DataType::FLOAT || rightType == DataType::FLOAT) {
                return DataType::FLOAT;
            }
            return DataType::INT;
        }
        
        return DataType::BOOLEAN;
    }
    
    return DataType::UNKNOWN;
}

bool SemanticAnalyzer::areTypesCompatible(DataType t1, DataType t2) {
    if (t1 == DataType::UNKNOWN || t2 == DataType::UNKNOWN) {
        return true; // Unknown types are compatible
    }
    
    if (t1 == t2) return true;
    
    // Numeric types are compatible
    if ((t1 == DataType::INT || t1 == DataType::FLOAT) &&
        (t2 == DataType::INT || t2 == DataType::FLOAT)) {
        return true;
    }
    
    return false;
}

void SemanticAnalyzer::analyzeSelectStatement(std::shared_ptr<SelectStatement> stmt) {
    if (!stmt) return;
    
    // Analyze table references
    if (stmt->fromClause) {
        analyzeTableReferences(stmt->fromClause);
    }
    
    // Analyze select list expressions
    if (stmt->selectList) {
        for (const auto& expr : stmt->selectList->expressions) {
            analyzeExpression(expr);
        }
    }
    
    // Analyze WHERE clause
    if (stmt->whereClause) {
        analyzeExpression(stmt->whereClause->condition);
    }
    
    // Analyze GROUP BY clause
    if (stmt->groupByClause) {
        for (const auto& expr : stmt->groupByClause->groupExpressions) {
            analyzeExpression(expr);
        }
        if (stmt->groupByClause->havingCondition) {
            analyzeExpression(stmt->groupByClause->havingCondition);
        }
    }
    
    // Analyze ORDER BY clause
    if (stmt->orderByClause) {
        for (const auto& [expr, isAsc] : stmt->orderByClause->orderItems) {
            analyzeExpression(expr);
        }
    }
}

void SemanticAnalyzer::analyzeInsertStatement(std::shared_ptr<InsertStatement> stmt) {
    if (!stmt) return;
    
    validateTableExists(stmt->tableName, 0, 0);
    
    // Validate column names
    for (const auto& col : stmt->columnNames) {
        validateColumnExists(stmt->tableName, col, 0, 0);
    }
    
    // Validate values
    for (const auto& valueRow : stmt->valuesList) {
        for (const auto& value : valueRow) {
            analyzeExpression(value);
        }
        
        // Check column count matches value count
        if (!stmt->columnNames.empty() && valueRow.size() != stmt->columnNames.size()) {
            errorHandler->addSemanticError(0, 0,
                                          "Column count does not match value count in INSERT statement");
        }
    }
}

void SemanticAnalyzer::analyzeUpdateStatement(std::shared_ptr<UpdateStatement> stmt) {
    if (!stmt) return;
    
    validateTableExists(stmt->tableName, 0, 0);
    
    // Validate assignments
    for (const auto& [colName, value] : stmt->assignments) {
        validateColumnExists(stmt->tableName, colName, 0, 0);
        analyzeExpression(value);
    }
    
    // Validate WHERE clause
    if (stmt->whereCondition) {
        analyzeExpression(stmt->whereCondition);
    }
}

void SemanticAnalyzer::analyzeDeleteStatement(std::shared_ptr<DeleteStatement> stmt) {
    if (!stmt) return;
    
    validateTableExists(stmt->tableName, 0, 0);
    
    // Validate WHERE clause
    if (stmt->whereCondition) {
        analyzeExpression(stmt->whereCondition);
    }
}

// Visitor implementations
std::string SemanticAnalyzer::visit(Literal* node) {
    return "";
}

std::string SemanticAnalyzer::visit(Identifier* node) {
    // Check if it's a column reference
    if (!node->tableAlias.empty()) {
        validateColumnExists(node->tableAlias, node->name, 0, 0);
    }
    return "";
}

std::string SemanticAnalyzer::visit(BinaryOp* node) {
    if (node->left) analyzeExpression(node->left);
    if (node->right) analyzeExpression(node->right);
    return "";
}

std::string SemanticAnalyzer::visit(UnaryOp* node) {
    if (node->operand) analyzeExpression(node->operand);
    return "";
}

std::string SemanticAnalyzer::visit(FunctionCall* node) {
    for (const auto& arg : node->arguments) {
        analyzeExpression(arg);
    }
    return "";
}

std::string SemanticAnalyzer::visit(CaseExpression* node) {
    for (const auto& [condition, result] : node->whenThen) {
        analyzeExpression(condition);
        analyzeExpression(result);
    }
    if (node->elseExpr) {
        analyzeExpression(node->elseExpr);
    }
    return "";
}

std::string SemanticAnalyzer::visit(SelectList* node) {
    for (const auto& expr : node->expressions) {
        analyzeExpression(expr);
    }
    return "";
}

std::string SemanticAnalyzer::visit(WhereClause* node) {
    if (node->condition) {
        analyzeExpression(node->condition);
    }
    return "";
}

std::string SemanticAnalyzer::visit(GroupByClause* node) {
    for (const auto& expr : node->groupExpressions) {
        analyzeExpression(expr);
    }
    if (node->havingCondition) {
        analyzeExpression(node->havingCondition);
    }
    return "";
}

std::string SemanticAnalyzer::visit(OrderByClause* node) {
    for (const auto& [expr, isAsc] : node->orderItems) {
        analyzeExpression(expr);
    }
    return "";
}

std::string SemanticAnalyzer::visit(SelectStatement* node) {
    analyzeSelectStatement(std::dynamic_pointer_cast<SelectStatement>(std::shared_ptr<ASTNode>(nullptr)));
    return "";
}

std::string SemanticAnalyzer::visit(InsertStatement* node) {
    auto stmt = std::make_shared<InsertStatement>(*node);
    analyzeInsertStatement(stmt);
    return "";
}

std::string SemanticAnalyzer::visit(UpdateStatement* node) {
    auto stmt = std::make_shared<UpdateStatement>(*node);
    analyzeUpdateStatement(stmt);
    return "";
}

std::string SemanticAnalyzer::visit(DeleteStatement* node) {
    auto stmt = std::make_shared<DeleteStatement>(*node);
    analyzeDeleteStatement(stmt);
    return "";
}

std::string SemanticAnalyzer::visit(TransactionStatement* node) {
    return "";
}

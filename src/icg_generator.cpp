#include "icg_generator.h"
#include <algorithm>
#include <sstream>
#include <iomanip>

ICGGenerator::ICGGenerator(std::shared_ptr<ErrorHandler> errHandler)
    : errorHandler(errHandler), indentLevel(0) {}

void ICGGenerator::indent() {
    indentLevel++;
}

void ICGGenerator::unindent() {
    if (indentLevel > 0) indentLevel--;
}

std::string ICGGenerator::getIndentation() const {
    return std::string(indentLevel * 2, ' ');
}

std::string ICGGenerator::sqlOpToMongoOp(const std::string& sqlOp) const {
    if (sqlOp == "=") return "$eq";
    if (sqlOp == "!=" || sqlOp == "<>") return "$ne";
    if (sqlOp == "<") return "$lt";
    if (sqlOp == ">") return "$gt";
    if (sqlOp == "<=") return "$lte";
    if (sqlOp == ">=") return "$gte";
    if (sqlOp == "LIKE") return "$regex";
    if (sqlOp == "IN") return "$in";
    if (sqlOp == "+") return "$add";
    if (sqlOp == "-") return "$subtract";
    if (sqlOp == "*") return "$multiply";
    if (sqlOp == "/") return "$divide";
    return sqlOp;
}

std::string ICGGenerator::expressionToMongoDB(const std::shared_ptr<Expression>& expr) {
    if (!expr) return "";
    
    if (auto lit = std::dynamic_pointer_cast<Literal>(expr)) {
        return literalToMongoDB(lit.get());
    }
    
    if (auto id = std::dynamic_pointer_cast<Identifier>(expr)) {
        return identifierToMongoDB(id.get());
    }
    
    if (auto binOp = std::dynamic_pointer_cast<BinaryOp>(expr)) {
        return binaryOpToMongoDB(binOp.get());
    }
    
    if (auto func = std::dynamic_pointer_cast<FunctionCall>(expr)) {
        return functionToMongoDB(func.get());
    }
    
    return "";
}

std::string ICGGenerator::literalToMongoDB(const Literal* lit) {
    switch (lit->type) {
        case Literal::Type::NUMBER:
            return lit->value;
        case Literal::Type::STRING:
            return "\"" + lit->value + "\"";
        case Literal::Type::BOOLEAN:
            return (lit->value == "true" || lit->value == "TRUE") ? "true" : "false";
        case Literal::Type::NULL_VAL:
            return "null";
    }
    return "";
}

std::string ICGGenerator::identifierToMongoDB(const Identifier* id) {
    if (!id->tableAlias.empty()) {
        return "\"$" + id->tableAlias + "." + id->name + "\"";
    }
    return "\"$" + id->name + "\"";
}

std::string ICGGenerator::functionToMongoDB(const FunctionCall* func) {
    std::string result;
    
    if (func->functionName == "COUNT") {
        return "{ \"$sum\": 1 }";
    } else if (func->functionName == "SUM") {
        if (!func->arguments.empty()) {
            return "{ \"$sum\": " + expressionToMongoDB(func->arguments[0]) + " }";
        }
    } else if (func->functionName == "AVG") {
        if (!func->arguments.empty()) {
            return "{ \"$avg\": " + expressionToMongoDB(func->arguments[0]) + " }";
        }
    } else if (func->functionName == "MIN") {
        if (!func->arguments.empty()) {
            return "{ \"$min\": " + expressionToMongoDB(func->arguments[0]) + " }";
        }
    } else if (func->functionName == "MAX") {
        if (!func->arguments.empty()) {
            return "{ \"$max\": " + expressionToMongoDB(func->arguments[0]) + " }";
        }
    }
    
    return "";
}

std::string ICGGenerator::binaryOpToMongoDB(const BinaryOp* op) {
    if (op->op == "AND") {
        return "{ \"$and\": [" + expressionToMongoDB(op->left) + ", " +
               expressionToMongoDB(op->right) + "] }";
    } else if (op->op == "OR") {
        return "{ \"$or\": [" + expressionToMongoDB(op->left) + ", " +
               expressionToMongoDB(op->right) + "] }";
    } else {
        std::string mongoOp = sqlOpToMongoOp(op->op);
        std::string left = expressionToMongoDB(op->left);
        std::string right = expressionToMongoDB(op->right);
        return "{ \"" + mongoOp + "\": [" + left + ", " + right + "] }";
    }
}

std::string ICGGenerator::buildMatchStage(const std::shared_ptr<Expression>& condition) {
    if (!condition) return "";
    
    return getIndentation() + "{\n" + getIndentation() + "  \"$match\": " +
           expressionToMongoDB(condition) + "\n" + getIndentation() + "}";
}

std::string ICGGenerator::buildProjectStage(const std::shared_ptr<SelectList>& selectList) {
    if (!selectList) return "";
    
    std::string proj = "{\n" + getIndentation() + "  \"$project\": {\n";
    
    for (size_t i = 0; i < selectList->expressions.size(); i++) {
        auto expr = selectList->expressions[i];
        
        if (auto id = std::dynamic_pointer_cast<Identifier>(expr)) {
            if (id->name == "*") {
                proj += getIndentation() + "    \"_id\": 1";
            } else {
                std::string fieldName = id->alias.empty() ? id->name : id->alias;
                proj += getIndentation() + "    \"" + fieldName + "\": 1";
            }
        } else {
            proj += getIndentation() + "    \"field_" + std::to_string(i) + "\": " +
                    expressionToMongoDB(expr);
        }
        
        if (i < selectList->expressions.size() - 1) {
            proj += ",";
        }
        proj += "\n";
    }
    
    proj += getIndentation() + "  }\n" + getIndentation() + "}";
    return proj;
}

std::string ICGGenerator::buildGroupStage(const std::shared_ptr<GroupByClause>& groupBy) {
    if (!groupBy) return "";
    
    std::string stage = "{\n" + getIndentation() + "  \"$group\": {\n";
    stage += getIndentation() + "    \"_id\": ";
    
    if (groupBy->groupExpressions.size() == 1) {
        stage += expressionToMongoDB(groupBy->groupExpressions[0]);
    } else {
        stage += "{\n";
        for (size_t i = 0; i < groupBy->groupExpressions.size(); i++) {
            stage += getIndentation() + "      \"field_" + std::to_string(i) + "\": " +
                    expressionToMongoDB(groupBy->groupExpressions[i]);
            if (i < groupBy->groupExpressions.size() - 1) stage += ",";
            stage += "\n";
        }
        stage += getIndentation() + "    }";
    }
    
    if (groupBy->havingCondition) {
        stage += ",\n" + getIndentation() + "    \"_having\": " +
                expressionToMongoDB(groupBy->havingCondition);
    }
    
    stage += "\n" + getIndentation() + "  }\n" + getIndentation() + "}";
    return stage;
}

std::string ICGGenerator::buildSortStage(const std::shared_ptr<OrderByClause>& orderBy) {
    if (!orderBy) return "";
    
    std::string stage = "{\n" + getIndentation() + "  \"$sort\": {\n";
    
    for (size_t i = 0; i < orderBy->orderItems.size(); i++) {
        auto [expr, isAsc] = orderBy->orderItems[i];
        
        std::string fieldName;
        if (auto id = std::dynamic_pointer_cast<Identifier>(expr)) {
            fieldName = "\"" + id->name + "\"";
        } else {
            fieldName = "\"field_" + std::to_string(i) + "\"";
        }
        
        stage += getIndentation() + "    " + fieldName + ": " +
                (isAsc ? "1" : "-1");
        
        if (i < orderBy->orderItems.size() - 1) {
            stage += ",";
        }
        stage += "\n";
    }
    
    stage += getIndentation() + "  }\n" + getIndentation() + "}";
    return stage;
}

std::string ICGGenerator::buildLimitStage(int limit) {
    if (limit <= 0) return "";
    
    return getIndentation() + "{\n" + getIndentation() + "  \"$limit\": " +
           std::to_string(limit) + "\n" + getIndentation() + "}";
}

std::string ICGGenerator::generateForSelect(std::shared_ptr<SelectStatement> stmt) {
    if (!stmt) return "";
    
    clearOutput();
    output << "// MongoDB Aggregation Pipeline\n";
    output << "db." << stmt->fromClause->tableName << ".aggregate([\n";
    
    indent();
    
    // Match stage (WHERE)
    if (stmt->whereClause) {
        output << buildMatchStage(stmt->whereClause->condition);
        
        if (stmt->groupByClause || stmt->orderByClause || stmt->limitValue > 0) {
            output << ",\n";
        } else {
            output << "\n";
        }
    }
    
    // Group stage (GROUP BY)
    if (stmt->groupByClause) {
        output << buildGroupStage(stmt->groupByClause);
        
        if (stmt->orderByClause || stmt->limitValue > 0) {
            output << ",\n";
        } else {
            output << "\n";
        }
    }
    
    // Project stage (SELECT)
    if (stmt->selectList) {
        output << buildProjectStage(stmt->selectList);
        
        if (stmt->orderByClause || stmt->limitValue > 0) {
            output << ",\n";
        } else {
            output << "\n";
        }
    }
    
    // Sort stage (ORDER BY)
    if (stmt->orderByClause) {
        output << buildSortStage(stmt->orderByClause);
        
        if (stmt->limitValue > 0) {
            output << ",\n";
        } else {
            output << "\n";
        }
    }
    
    // Limit stage (LIMIT)
    if (stmt->limitValue > 0) {
        output << buildLimitStage(stmt->limitValue) << "\n";
    }
    
    unindent();
    output << "])\n";
    
    return getOutput();
}

std::string ICGGenerator::generateForInsert(std::shared_ptr<InsertStatement> stmt) {
    if (!stmt) return "";
    
    clearOutput();
    output << "// MongoDB Insert Operation\n";
    
    for (size_t i = 0; i < stmt->valuesList.size(); i++) {
        output << "db." << stmt->tableName << ".insertOne({\n";
        
        const auto& values = stmt->valuesList[i];
        
        for (size_t j = 0; j < stmt->columnNames.size() && j < values.size(); j++) {
            output << "  \"" << stmt->columnNames[j] << "\": " 
                   << expressionToMongoDB(values[j]);
            
            if (j < stmt->columnNames.size() - 1) {
                output << ",";
            }
            output << "\n";
        }
        
        output << "})\n";
        
        if (i < stmt->valuesList.size() - 1) {
            output << "\n";
        }
    }
    
    return getOutput();
}

std::string ICGGenerator::generateForUpdate(std::shared_ptr<UpdateStatement> stmt) {
    if (!stmt) return "";
    
    clearOutput();
    output << "// MongoDB Update Operation\n";
    output << "db." << stmt->tableName << ".updateMany(\n";
    
    // Filter
    output << "  ";
    if (stmt->whereCondition) {
        output << expressionToMongoDB(stmt->whereCondition);
    } else {
        output << "{}";
    }
    output << ",\n";
    
    // Update
    output << "  {\n    \"$set\": {\n";
    
    for (size_t i = 0; i < stmt->assignments.size(); i++) {
        auto [col, val] = stmt->assignments[i];
        output << "      \"" << col << "\": " << expressionToMongoDB(val);
        
        if (i < stmt->assignments.size() - 1) {
            output << ",";
        }
        output << "\n";
    }
    
    output << "    }\n  }\n";
    output << ")\n";
    
    return getOutput();
}

std::string ICGGenerator::generateForDelete(std::shared_ptr<DeleteStatement> stmt) {
    if (!stmt) return "";
    
    clearOutput();
    output << "// MongoDB Delete Operation\n";
    output << "db." << stmt->tableName << ".deleteMany(\n";
    
    output << "  ";
    if (stmt->whereCondition) {
        output << expressionToMongoDB(stmt->whereCondition);
    } else {
        output << "{}";
    }
    output << "\n";
    
    output << ")\n";
    
    return getOutput();
}

std::string ICGGenerator::generateForTransaction(std::shared_ptr<TransactionStatement> stmt) {
    if (!stmt) return "";
    
    clearOutput();
    
    switch (stmt->type) {
        case TransactionStatement::Type::BEGIN:
            output << "// Begin Transaction\n";
            output << "session.startTransaction()\n";
            break;
        case TransactionStatement::Type::COMMIT:
            output << "// Commit Transaction\n";
            output << "session.commitTransaction()\n";
            break;
        case TransactionStatement::Type::ROLLBACK:
            output << "// Rollback Transaction\n";
            output << "session.abortTransaction()\n";
            break;
    }
    
    return getOutput();
}

// Visitor implementations
std::string ICGGenerator::visit(Literal* node) {
    return literalToMongoDB(node);
}

std::string ICGGenerator::visit(Identifier* node) {
    return identifierToMongoDB(node);
}

std::string ICGGenerator::visit(BinaryOp* node) {
    return binaryOpToMongoDB(node);
}

std::string ICGGenerator::visit(UnaryOp* node) {
    return "{ \"$not\": " + expressionToMongoDB(node->operand) + " }";
}

std::string ICGGenerator::visit(FunctionCall* node) {
    return functionToMongoDB(node);
}

std::string ICGGenerator::visit(CaseExpression* node) {
    std::string result = "{ \"$cond\": {\n";
    result += "    \"if\": ";
    
    if (!node->whenThen.empty()) {
        result += expressionToMongoDB(node->whenThen[0].first) + ",\n";
        result += "    \"then\": " + expressionToMongoDB(node->whenThen[0].second) + ",\n";
    }
    
    result += "    \"else\": ";
    if (node->elseExpr) {
        result += expressionToMongoDB(node->elseExpr);
    } else {
        result += "null";
    }
    result += "\n  }\n}";
    
    return result;
}

std::string ICGGenerator::visit(SelectList* node) {
    return "";  // Handled in buildProjectStage
}

std::string ICGGenerator::visit(WhereClause* node) {
    return "";  // Handled in buildMatchStage
}

std::string ICGGenerator::visit(GroupByClause* node) {
    return "";  // Handled in buildGroupStage
}

std::string ICGGenerator::visit(OrderByClause* node) {
    return "";  // Handled in buildSortStage
}

std::string ICGGenerator::visit(SelectStatement* node) {
    return generateForSelect(std::dynamic_pointer_cast<SelectStatement>(std::shared_ptr<ASTNode>(nullptr)));
}

std::string ICGGenerator::visit(InsertStatement* node) {
    return "";  // Handled in generateForInsert
}

std::string ICGGenerator::visit(UpdateStatement* node) {
    return "";  // Handled in generateForUpdate
}

std::string ICGGenerator::visit(DeleteStatement* node) {
    return "";  // Handled in generateForDelete
}

std::string ICGGenerator::visit(TransactionStatement* node) {
    return "";  // Handled in generateForTransaction
}

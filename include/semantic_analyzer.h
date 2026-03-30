#ifndef SEMANTIC_ANALYZER_H
#define SEMANTIC_ANALYZER_H

#include "ast.h"
#include "symbol_table.h"
#include "error_handler.h"
#include <memory>

class SemanticAnalyzer : public ASTVisitor {
private:
    std::shared_ptr<SymbolTable> symbolTable;
    std::shared_ptr<ErrorHandler> errorHandler;
    
    // Analysis helpers
    void analyzeTableReferences(const std::shared_ptr<TableReference>& tableRef);
    void analyzeExpression(const std::shared_ptr<Expression>& expr);
    void validateTableExists(const std::string& tableName, int line, int col);
    void validateColumnExists(const std::string& tableName, const std::string& columnName, int line, int col);
    DataType inferExpressionType(const std::shared_ptr<Expression>& expr);
    
    // Type checking
    bool areTypesCompatible(DataType t1, DataType t2);
    
public:
    SemanticAnalyzer(std::shared_ptr<SymbolTable> symTable, 
                     std::shared_ptr<ErrorHandler> errHandler);
    
    void analyze(std::shared_ptr<ASTNode> ast);
    void analyzeSelectStatement(std::shared_ptr<SelectStatement> stmt);
    void analyzeInsertStatement(std::shared_ptr<InsertStatement> stmt);
    void analyzeUpdateStatement(std::shared_ptr<UpdateStatement> stmt);
    void analyzeDeleteStatement(std::shared_ptr<DeleteStatement> stmt);
    
    // ASTVisitor implementation
    std::string visit(Literal* node) override;
    std::string visit(Identifier* node) override;
    std::string visit(BinaryOp* node) override;
    std::string visit(UnaryOp* node) override;
    std::string visit(FunctionCall* node) override;
    std::string visit(CaseExpression* node) override;
    std::string visit(SelectList* node) override;
    std::string visit(WhereClause* node) override;
    std::string visit(GroupByClause* node) override;
    std::string visit(OrderByClause* node) override;
    std::string visit(SelectStatement* node) override;
    std::string visit(InsertStatement* node) override;
    std::string visit(UpdateStatement* node) override;
    std::string visit(DeleteStatement* node) override;
    std::string visit(TransactionStatement* node) override;
    
    // Getters
    bool hasErrors() const { return errorHandler->hasErrors(); }
    bool hasCriticalErrors() const { return errorHandler->hasCriticalErrors(); }
};

#endif // SEMANTIC_ANALYZER_H

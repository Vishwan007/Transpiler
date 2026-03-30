#ifndef ICG_GENERATOR_H
#define ICG_GENERATOR_H

#include "ast.h"
#include "error_handler.h"
#include <string>
#include <memory>
#include <vector>
#include <sstream>

/**
 * Intermediate Code Generator (ICG) for MongoDB
 * Generates MongoDB JSON queries from the validated AST
 */
class ICGGenerator : public ASTVisitor {
private:
    std::shared_ptr<ErrorHandler> errorHandler;
    std::stringstream output;
    int indentLevel;
    
    // Helper functions
    void indent();
    void unindent();
    std::string getIndentation() const;
    
    // MongoDB query building helpers
    std::string buildMatchStage(const std::shared_ptr<Expression>& condition);
    std::string buildProjectStage(const std::shared_ptr<SelectList>& selectList);
    std::string buildGroupStage(const std::shared_ptr<GroupByClause>& groupBy);
    std::string buildSortStage(const std::shared_ptr<OrderByClause>& orderBy);
    std::string buildLimitStage(int limit);
    
    // Expression to MongoDB conversion
    std::string expressionToMongoDB(const std::shared_ptr<Expression>& expr);
    std::string binaryOpToMongoDB(const BinaryOp* op);
    std::string functionToMongoDB(const FunctionCall* func);
    std::string identifierToMongoDB(const Identifier* id);
    std::string literalToMongoDB(const Literal* lit);
    
    // Operator conversion
    std::string sqlOpToMongoOp(const std::string& sqlOp) const;
    
public:
    ICGGenerator(std::shared_ptr<ErrorHandler> errHandler);
    
    // Main generation methods
    std::string generateForSelect(std::shared_ptr<SelectStatement> stmt);
    std::string generateForInsert(std::shared_ptr<InsertStatement> stmt);
    std::string generateForUpdate(std::shared_ptr<UpdateStatement> stmt);
    std::string generateForDelete(std::shared_ptr<DeleteStatement> stmt);
    std::string generateForTransaction(std::shared_ptr<TransactionStatement> stmt);
    
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
    
    std::string getOutput() const { return output.str(); }
    void clearOutput() { output.str(""); output.clear(); }
};

#endif // ICG_GENERATOR_H

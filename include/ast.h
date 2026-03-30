#ifndef AST_H
#define AST_H

#include <string>
#include <vector>
#include <memory>

// Forward declarations
class ASTNode;
class SelectStatement;
class InsertStatement;
class UpdateStatement;
class DeleteStatement;
class CreateTableStatement;
class TransactionStatement;

using ASTNodePtr = std::shared_ptr<ASTNode>;

// Base AST Node
class ASTNode {
public:
    virtual ~ASTNode() = default;
    virtual std::string accept(class ASTVisitor* visitor) = 0;
};

// Expressions
class Expression : public ASTNode {};

class Literal : public Expression {
public:
    std::string value;
    enum class Type { NUMBER, STRING, NULL_VAL, BOOLEAN } type;
    
    Literal(const std::string& v, Type t) : value(v), type(t) {}
    std::string accept(class ASTVisitor* visitor) override;
};

class Identifier : public Expression {
public:
    std::string name;
    std::string alias;
    std::string tableAlias;  // For qualified references like table.column
    
    Identifier(const std::string& n) : name(n), alias("") {}
    std::string accept(class ASTVisitor* visitor) override;
};

class BinaryOp : public Expression {
public:
    std::shared_ptr<Expression> left;
    std::shared_ptr<Expression> right;
    std::string op;
    
    BinaryOp(std::shared_ptr<Expression> l, const std::string& o, std::shared_ptr<Expression> r)
        : left(l), op(o), right(r) {}
    std::string accept(class ASTVisitor* visitor) override;
};

class UnaryOp : public Expression {
public:
    std::shared_ptr<Expression> operand;
    std::string op;
    
    UnaryOp(const std::string& o, std::shared_ptr<Expression> expr)
        : op(o), operand(expr) {}
    std::string accept(class ASTVisitor* visitor) override;
};

class FunctionCall : public Expression {
public:
    std::string functionName;
    std::vector<std::shared_ptr<Expression>> arguments;
    bool isAggregate;
    
    FunctionCall(const std::string& name, bool agg = false)
        : functionName(name), isAggregate(agg) {}
    std::string accept(class ASTVisitor* visitor) override;
};

class CaseExpression : public Expression {
public:
    std::vector<std::pair<std::shared_ptr<Expression>, std::shared_ptr<Expression>>> whenThen;
    std::shared_ptr<Expression> elseExpr;
    
    std::string accept(class ASTVisitor* visitor) override;
};

// Table references
class TableReference {
public:
    std::string tableName;
    std::string alias;
    std::vector<std::shared_ptr<TableReference>> joins;
    std::shared_ptr<Expression> joinCondition;
    std::string joinType;  // INNER, LEFT, RIGHT, FULL
    
    TableReference(const std::string& name) : tableName(name), alias(name) {}
    virtual ~TableReference() = default;
};

// SELECT specific
class SelectList : public ASTNode {
public:
    std::vector<std::shared_ptr<Expression>> expressions;
    bool isDistinct;
    
    SelectList(bool distinct = false) : isDistinct(distinct) {}
    std::string accept(class ASTVisitor* visitor) override;
};

class WhereClause : public ASTNode {
public:
    std::shared_ptr<Expression> condition;
    
    WhereClause(std::shared_ptr<Expression> cond) : condition(cond) {}
    std::string accept(class ASTVisitor* visitor) override;
};

class GroupByClause : public ASTNode {
public:
    std::vector<std::shared_ptr<Expression>> groupExpressions;
    std::shared_ptr<Expression> havingCondition;
    
    std::string accept(class ASTVisitor* visitor) override;
};

class OrderByClause : public ASTNode {
public:
    std::vector<std::pair<std::shared_ptr<Expression>, bool>> orderItems;  // (expr, isAsc)
    
    std::string accept(class ASTVisitor* visitor) override;
};

class SelectStatement : public ASTNode {
public:
    std::shared_ptr<SelectList> selectList;
    std::shared_ptr<TableReference> fromClause;
    std::shared_ptr<WhereClause> whereClause;
    std::shared_ptr<GroupByClause> groupByClause;
    std::shared_ptr<OrderByClause> orderByClause;
    int limitValue;
    
    SelectStatement() : limitValue(-1) {}
    std::string accept(class ASTVisitor* visitor) override;
};

// INSERT statement
class InsertStatement : public ASTNode {
public:
    std::string tableName;
    std::vector<std::string> columnNames;
    std::vector<std::vector<std::shared_ptr<Expression>>> valuesList;
    
    std::string accept(class ASTVisitor* visitor) override;
};

// UPDATE statement
class UpdateStatement : public ASTNode {
public:
    std::string tableName;
    std::vector<std::pair<std::string, std::shared_ptr<Expression>>> assignments;
    std::shared_ptr<Expression> whereCondition;
    
    std::string accept(class ASTVisitor* visitor) override;
};

// DELETE statement
class DeleteStatement : public ASTNode {
public:
    std::string tableName;
    std::shared_ptr<Expression> whereCondition;
    
    std::string accept(class ASTVisitor* visitor) override;
};

// CREATE TABLE statement
struct ColumnDef {
    std::string name;
    std::string type;
    int length;
    bool isNullable;
    bool isPrimaryKey;

    ColumnDef() : length(-1), isNullable(true), isPrimaryKey(false) {}
};

class CreateTableStatement : public ASTNode {
public:
    std::string tableName;
    std::vector<ColumnDef> columns;

    std::string accept(class ASTVisitor* visitor) override;
};

// Transaction statements
class TransactionStatement : public ASTNode {
public:
    enum class Type { BEGIN, COMMIT, ROLLBACK };
    Type type;
    
    TransactionStatement(Type t) : type(t) {}
    std::string accept(class ASTVisitor* visitor) override;
};

// Visitor pattern for AST traversal
class ASTVisitor {
public:
    virtual ~ASTVisitor() = default;
    virtual std::string visit(Literal* node) = 0;
    virtual std::string visit(Identifier* node) = 0;
    virtual std::string visit(BinaryOp* node) = 0;
    virtual std::string visit(UnaryOp* node) = 0;
    virtual std::string visit(FunctionCall* node) = 0;
    virtual std::string visit(CaseExpression* node) = 0;
    virtual std::string visit(SelectList* node) = 0;
    virtual std::string visit(WhereClause* node) = 0;
    virtual std::string visit(GroupByClause* node) = 0;
    virtual std::string visit(OrderByClause* node) = 0;
    virtual std::string visit(SelectStatement* node) = 0;
    virtual std::string visit(InsertStatement* node) = 0;
    virtual std::string visit(UpdateStatement* node) = 0;
    virtual std::string visit(DeleteStatement* node) = 0;
    virtual std::string visit(CreateTableStatement* node) = 0;
    virtual std::string visit(TransactionStatement* node) = 0;
};

#endif // AST_H

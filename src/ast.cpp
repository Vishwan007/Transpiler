#include "ast.h"

// Literal
std::string Literal::accept(ASTVisitor* visitor) {
    return visitor->visit(this);
}

// Identifier
std::string Identifier::accept(ASTVisitor* visitor) {
    return visitor->visit(this);
}

// BinaryOp
std::string BinaryOp::accept(ASTVisitor* visitor) {
    return visitor->visit(this);
}

// UnaryOp
std::string UnaryOp::accept(ASTVisitor* visitor) {
    return visitor->visit(this);
}

// FunctionCall
std::string FunctionCall::accept(ASTVisitor* visitor) {
    return visitor->visit(this);
}

// CaseExpression
std::string CaseExpression::accept(ASTVisitor* visitor) {
    return visitor->visit(this);
}

// SelectList
std::string SelectList::accept(ASTVisitor* visitor) {
    return visitor->visit(this);
}

// WhereClause
std::string WhereClause::accept(ASTVisitor* visitor) {
    return visitor->visit(this);
}

// GroupByClause
std::string GroupByClause::accept(ASTVisitor* visitor) {
    return visitor->visit(this);
}

// OrderByClause
std::string OrderByClause::accept(ASTVisitor* visitor) {
    return visitor->visit(this);
}

// SelectStatement
std::string SelectStatement::accept(ASTVisitor* visitor) {
    return visitor->visit(this);
}

// InsertStatement
std::string InsertStatement::accept(ASTVisitor* visitor) {
    return visitor->visit(this);
}

// UpdateStatement
std::string UpdateStatement::accept(ASTVisitor* visitor) {
    return visitor->visit(this);
}

// DeleteStatement
std::string DeleteStatement::accept(ASTVisitor* visitor) {
    return visitor->visit(this);
}

// TransactionStatement
std::string TransactionStatement::accept(ASTVisitor* visitor) {
    return visitor->visit(this);
}

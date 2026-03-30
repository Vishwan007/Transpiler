# SQL to NoSQL Compiler Architecture

## Overview

This document describes the internal architecture and design of the SQL-to-NoSQL transcompiler. The compiler uses a classic multi-phase architecture with comprehensive error handling at each stage.

## Compiler Pipeline

```
SQL Source Code
    │
    ├─ [PHASE 1] Lexer (Lexical Analysis)
    │     ├─ Character-by-character scanning
    │     ├─ Token recognition
    │     └─ ERROR: Unexpected char, malformed literal
    │
    ├─ [PHASE 2] Parser (Syntax Analysis)
    │     ├─ Token stream → AST
    │     ├─ Grammar validation
    │     └─ ERROR: Missing keyword, malformed expression
    │
    ├─ [PHASE 3] Semantic Analyzer (Type & Reference Check)
    │     ├─ Type validation
    │     ├─ Table/column existence check
    │     └─ ERROR: Undefined table, type mismatch
    │
    └─ [PHASE 4] ICG Generator (Intermediate Code Generation)
          ├─ AST → MongoDB JavaScript
          ├─ Aggregation pipeline construction
          └─ ERROR: Code generation failure
    
    MongoDB JavaScript Output
```

## Detailed Component Description

### 1. Lexer (Lexical Analysis)

**Purpose**: Convert character stream into token stream

**File**: `lexer.h`, `lexer.cpp`

**Key Responsibilities**:
- Skip whitespace and comments
- Recognize keywords (case-insensitive)
- Parse numbers (integers and floats)
- Parse string literals with escape sequences
- Parse identifiers and operators
- Track line and column numbers for error reporting

**Data Structures**:
```cpp
struct Token {
    TokenType type;        // What kind of token
    std::string value;     // The actual text
    int line;              // For error reporting
    int column;            // For error reporting
};

enum class TokenType {
    // Keywords
    SELECT, FROM, WHERE, GROUP, BY, ...
    // Operators
    PLUS, MINUS, EQUAL, ...
    // Literals
    NUMBER, STRING, IDENTIFIER, ...
};
```

**Algorithm**:
1. Skip whitespace and comments
2. Check character type:
   - Digit → readNumber()
   - Quote → readString()
   - Letter → readIdentifierOrKeyword()
   - Operator char → readOperator()
   - Delimiter → create token
3. Store token with position info
4. Report lexical errors (unexpected chars, unterminated strings)

**Error Handling**:
- Collects LEXICAL_ERROR for invalid tokens
- Tracks exact position (line, column)
- Continues scanning to find multiple errors

### 2. Parser (Syntax Analysis)

**Purpose**: Build Abstract Syntax Tree (AST) from token stream

**File**: `parser.h`, `parser.cpp`

**Key Concepts**:
- **Recursive Descent Parser**: Each grammar rule is a function
- **Operator Precedence**: Handled via function nesting
- **Error Recovery**: Synchronization at statement boundaries

**Grammar Rules** (simplified):
```
statement   → selectStmt | insertStmt | updateStmt | deleteStmt | transactionStmt
selectStmt  → SELECT [DISTINCT] selectList FROM tableRef [WHERE condition] ...
expression  → logicalOr
logicalOr   → logicalAnd (OR logicalAnd)*
logicalAnd  → comparison (AND comparison)*
comparison  → additive (( = | != | < | > | <= | >= | LIKE | IN ) additive)?
additive    → multiplicative (( + | - ) multiplicative)*
multiplicative → unary (( * | / | % ) unary)*
unary       → (NOT | - | +)? primary
primary     → NUMBER | STRING | IDENTIFIER | ( expression ) | functionCall
```

**Key Methods**:
- `parseStatement()` - Determines statement type
- `parseSelectStatement()` - Handles SELECT
- `parseExpression()` - Parses expressions with precedence
- `synchronize()` - Error recovery

**Error Handling**:
- Reports SYNTAX_ERROR for grammar violations
- Synchronization points at statement boundaries
- Attempts to recover and continue parsing

**AST Structure**:
```cpp
class ASTNode {
    virtual std::string accept(ASTVisitor* visitor) = 0;
};

class Expression : public ASTNode {};
class Statement : public ASTNode {};

class SelectStatement : public ASTNode {
    SelectList* selectList;
    TableReference* fromClause;
    WhereClause* whereClause;
    GroupByClause* groupByClause;
    OrderByClause* orderByClause;
    int limitValue;
};
```

### 3. Symbol Table & Type System

**Purpose**: Track symbols and validate references

**File**: `symbol_table.h`, `symbol_table.cpp`

**Key Data Structures**:
```cpp
struct Symbol {
    std::string name;
    SymbolType type;      // TABLE, COLUMN, FUNCTION, VARIABLE, AGGREGATE
    DataType dataType;    // INT, FLOAT, STRING, DATETIME, JSON, etc.
    int lineNumber;       // For error reporting
};

class Scope {
    unordered_map<std::string, Symbol*> symbols;
    Scope* parentScope;   // For nested scopes
};

class SymbolTable {
    Scope* globalScope;
    Scope* currentScope;
    unordered_map<string, vector<string>> tableColumns;
    unordered_map<string, vector<DataType>> tableColumnTypes;
};
```

**Operations**:
- `addSymbol()` - Register new symbol
- `lookup()` - Find symbol (recursive in parent scopes)
- `registerTable()` - Register table with columns
- `isValidTable()` - Check table existence
- `isValidColumn()` - Check column existence
- `getColumnType()` - Return column's data type

**Pre-registered Tables**:
The symbol table initializes with:
```
users (id:INT, name:STRING, email:STRING, age:INT, created_at:DATETIME)
orders (id:INT, user_id:INT, total:FLOAT, status:STRING, created_at:DATETIME)
products (id:INT, name:STRING, price:FLOAT, stock:INT)
```

### 4. Semantic Analyzer

**Purpose**: Validate semantic correctness of AST

**File**: `semantic_analyzer.h`, `semantic_analyzer.cpp`

**Visitor Pattern**:
```cpp
class ASTVisitor {
    virtual std::string visit(Literal* node) = 0;
    virtual std::string visit(Identifier* node) = 0;
    virtual std::string visit(BinaryOp* node) = 0;
    // ... more visit methods
};

class SemanticAnalyzer : public ASTVisitor {
    // Implements all visit methods
};
```

**Validation Tasks**:
1. **Table Existence**: Verify all referenced tables exist
2. **Column Existence**: Verify all columns exist in tables
3. **Type Compatibility**: Check type conversions are valid
4. **Function Validity**: Verify aggregate functions are used correctly
5. **Reference Validity**: Ensure all identifiers are defined

**Error Detection**:
- Undefined tables: `"Table 'xyz' is not defined"`
- Undefined columns: `"Column 'abc' not in table 'xyz'"`
- Type mismatches: `"Cannot compare STRING with INT"`
- Invalid aggregates: `"COUNT(*) only valid with GROUP BY"`

### 5. Intermediate Code Generator (ICG)

**Purpose**: Generate MongoDB JavaScript from validated AST

**File**: `icg_generator.h`, `icg_generator.cpp`

**Key Concept**: MongoDB Aggregation Pipeline

SQL statements map to MongoDB operations:
```
SQL SELECT          → $project stage
SQL WHERE           → $match stage
SQL GROUP BY        → $group stage
SQL ORDER BY        → $sort stage
SQL LIMIT           → $limit stage
SQL INSERT          → insertOne/insertMany
SQL UPDATE          → updateMany
SQL DELETE          → deleteMany
```

**Code Generation Process**:

1. **SELECT→ Aggregation Pipeline**:
   ```
   SELECT col1, col2
   FROM table
   WHERE condition
   GROUP BY col1
   ORDER BY col2 DESC
   LIMIT 10
   ```
   
   Becomes:
   ```javascript
   db.table.aggregate([
     { $match: { condition } },
     { $group: { _id: "$col1" } },
     { $sort: { col2: -1 } },
     { $limit: 10 },
     { $project: { col1: 1, col2: 1 } }
   ])
   ```

2. **INSERT→ insertOne/insertMany**:
   ```sql
   INSERT INTO table (col1, col2)
   VALUES (val1, val2)
   ```
   
   Becomes:
   ```javascript
   db.table.insertOne({
     "col1": val1,
     "col2": val2
   })
   ```

3. **UPDATE→ updateMany**:
   ```sql
   UPDATE table SET col1=val1 WHERE condition
   ```
   
   Becomes:
   ```javascript
   db.table.updateMany(
     { condition },
     { $set: { col1: val1 } }
   )
   ```

4. **DELETE→ deleteMany**:
   ```sql
   DELETE FROM table WHERE condition
   ```
   
   Becomes:
   ```javascript
   db.table.deleteMany({ condition })
   ```

**Expression Conversion**:

SQL operators → MongoDB operators:
```
= → $eq
!= → $ne
< → $lt
> → $gt
<= → $lte
>= → $gte
AND → $and
OR → $or
LIKE → $regex
IN → $in
+ → $add
- → $subtract
* → $multiply
/ → $divide
```

### 6. Error Handler

**Purpose**: Centralized error collection and reporting

**File**: `error_handler.h`, `error_handler.cpp`

**Error Categories**:
```cpp
enum class ErrorLevel {
    LEXICAL_ERROR,      // Phase 1
    SYNTAX_ERROR,       // Phase 2
    SEMANTIC_ERROR,     // Phase 3
    TRANSLATION_ERROR   // Phase 4
};

enum class ErrorSeverity {
    WARNING,   // Non-blocking
    ERROR,     // Blocking
    FATAL      // Catastrophic
};

struct CompileError {
    ErrorLevel level;
    ErrorSeverity severity;
    int line;
    int column;
    std::string message;
    std::string context;  // Code snippet
};
```

**Key Methods**:
- `addError()` - Record error with position
- `hasErrors()` - Check for critical errors
- `hasCriticalErrors()` - Check for ERROR or FATAL
- `printErrors()` - Display all errors sorted by line
- `getErrorSummary()` - Statistics
- `canContinue()` - Decision to proceed to next phase

**Error Output Format**:
```
=== COMPILATION ERRORS AND WARNINGS ===

[ERROR] Semantic Error at line 1, column 25: Column 'id' does not exist
  Context: SELECT users.id FROM users

[WARNING] Type mismatch: INT and STRING

=== SUMMARY ===
Total Errors: 1
Total Warnings: 1
```

## Visitor Pattern for AST Traversal

The compiler uses the Visitor pattern for AST traversal:

```cpp
// In AST node:
class SelectStatement : public ASTNode {
    std::string accept(ASTVisitor* visitor) override {
        return visitor->visit(this);
    }
};

// In visitor:
class SemanticAnalyzer : public ASTVisitor {
    std::string visit(SelectStatement* node) override {
        // Analyze SELECT statement
        analyzeSelectStatement(node);
        return "";
    }
};

// Usage:
ast->accept(&analyzer);  // Double dispatch
```

Benefits:
- Clean separation between AST and analysis
- Easy to add new analysis passes
- Type-safe traversal

## Memory Management

All dynamic memory uses smart pointers:

```cpp
// Shared ownership - reference counted
std::shared_ptr<SelectStatement> stmt;
std::shared_ptr<Expression> expr;

// Unique ownership - exclusive access
std::unique_ptr<Token> token;

// Benefits:
// - No memory leaks
// - RAII principles
// - Exception safety
```

## Data Flow Example

### Input: Simple SELECT
```sql
SELECT id, name FROM users WHERE age > 25;
```

### Phase 1: Lexer Output
```
[SELECT] [IDENTIFIER:id] [COMMA] [IDENTIFIER:name]
[FROM] [IDENTIFIER:users] [WHERE] [IDENTIFIER:age]
[GREATER_THAN] [NUMBER:25] [EOF]
```

### Phase 2: Parser Output (AST)
```
SelectStatement
├── SelectList
│   ├── Identifier(id)
│   └── Identifier(name)
├── TableReference(users)
└── WhereClause
    └── BinaryOp
        ├── Identifier(age)
        ├── >
        └── Literal(25)
```

### Phase 3: Semantic Analysis
```
✓ Table 'users' exists
✓ Column 'id' in 'users'
✓ Column 'name' in 'users'
✓ Column 'age' in 'users'
✓ Type INT > NUMBER is valid
✓ No errors detected
```

### Phase 4: ICG Output
```javascript
// MongoDB Aggregation Pipeline
db.users.aggregate([
  {
    "$match": { "$gt": ["$age", 25] }
  },
  {
    "$project": {
      "id": 1,
      "name": 1
    }
  }
])
```

## Design Patterns Used

1. **Visitor Pattern** - AST traversal
2. **Factory Pattern** - Token creation
3. **Singleton Pattern** - Symbol table, error handler
4. **Decorator Pattern** - Error context
5. **Chain of Responsibility** - Error collection
6. **Strategy Pattern** - Different code generators

## Complexity Analysis

| Phase | Operation | Complexity |
|-------|-----------|------------|
| Lexer | Tokenization | O(n) |
| Parser | AST building | O(n) |
| Semantic | Validation | O(n) |
| ICG | Code generation | O(n) |
| **Total** | **Full compilation** | **O(n)** |

Where n = input size (character count)

## Symbol Table Lookup

Symbol table uses hash maps for O(1) average lookup:

```
Lookup "users":
- Hash("users") → bucket
- Check bucket for "users" symbol
- Return symbol or null
- Average: O(1)
```

## Error Recovery Strategies

### Lexer Recovery
- Skip invalid character
- Continue from next character
- Useful for finding multiple invalid tokens

### Parser Recovery
- Synchronize at statement boundaries
- Skip to next valid statement keyword
- Useful for finding multiple syntax errors

### Semantic Recovery
- Continue checking other expressions
- Accumulate errors
- Report all at once

## Extension Points

### Adding New SQL Feature
1. Update `TokenType` enum
2. Update `Lexer::keywords` map
3. Add parser rule in `Parser`
4. Add AST node if needed
5. Add semantic checks in `SemanticAnalyzer`
6. Add code generation in `ICGGenerator`

### Adding New Target Database
1. Create new generator class inheriting `ASTVisitor`
2. Implement all `visit()` methods
3. Update `main.cpp` to instantiate correct generator
4. Recompile

### Adding Optimization Pass
1. Create optimizer class inheriting `ASTVisitor`
2. Traverse AST and optimize
3. Return optimized AST
4. Insert between Semantic Analysis and ICG

## Performance Optimizations

1. **String Interning**: Keywords in hash map
2. **Error Batching**: Collect before reporting
3. **Hash Maps**: O(1) symbol lookup
4. **Early Termination**: Stop on catastrophic errors
5. **Single Pass**: No backtracking or rework

## Future Improvements

1. **Subquery Support**: Nested SELECT handling
2. **Index Selection**: Query optimization
3. **Query Plans**: Execution plan visualization
4. **Window Functions**: Advanced aggregations
5. **View Support**: Virtual table definitions
6. **Multi-target**: Support DynamoDB, Cassandra, etc.

---

For more information, see:
- `README.md` - Feature overview
- `BUILD.md` - Building and usage
- Header files - API documentation

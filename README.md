# SQL to NoSQL (MongoDB) Transcompiler

A comprehensive compiler written in C++17 that translates SQL queries to MongoDB JSON operations. This project demonstrates all phases of compiler design with complete error handling at each stage before intermediate code generation.

## Overview

This transcompiler implements a complete 4-phase compiler architecture:

1. **Lexical Analysis** - Tokenizes SQL input
2. **Syntax Analysis** - Builds Abstract Syntax Tree (AST)
3. **Semantic Analysis** - Validates types, tables, and references
4. **Intermediate Code Generation** - Generates MongoDB JavaScript code

All errors are collected at each phase and reported before proceeding to the next phase, ensuring comprehensive error feedback.

## Key Features

- **Complete Error Handling**: All errors reported at lexical, syntax, semantic, and translation levels before ICG
- **4-Phase Compiler Architecture**: Classic compiler phases with proper separation of concerns
- **SQL to MongoDB Translation**: Converts complex SQL queries to MongoDB aggregation pipelines
- **CLI Tool**: Command-line interface for file-based and interactive compilation
- **Comprehensive Testing**: Full test suite with lexer, parser, and integration tests
- **Well-Documented**: Detailed documentation on compiler architecture and usage

## Supported SQL Operations

### Queries
- ✅ SELECT with WHERE, GROUP BY, ORDER BY, LIMIT
- ✅ INSERT INTO ... VALUES (single and multiple rows)
- ✅ UPDATE ... SET ... WHERE
- ✅ DELETE FROM ... WHERE
- ✅ JOINs (INNER, LEFT, RIGHT, FULL OUTER)
- ✅ Aggregate functions (COUNT, SUM, AVG, MIN, MAX)
- ✅ DISTINCT, HAVING, CASE expressions
- ✅ Transaction statements (BEGIN, COMMIT, ROLLBACK)

### Operators
- ✅ Comparison: =, !=, <>, <, >, <=, >=
- ✅ Logical: AND, OR, NOT
- ✅ Arithmetic: +, -, *, /, %
- ✅ Patterns: LIKE, BETWEEN, IN, IS NULL

## Quick Start

### Build

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .
```

### Usage

**Interactive Mode:**
```bash
./transcompiler -i
```

**File-based Mode:**
```bash
./transcompiler -f query.sql -o output.mongodb
./transcompiler -f query.sql  # Output to stdout
```

**View Help:**
```bash
./transcompiler --help
```

## Compilation Phases Explained

### Phase 1: Lexical Analysis
**What it does**: Breaks SQL source code into tokens (keywords, identifiers, operators, literals)

**Errors detected**:
- Unexpected characters (e.g., `@`, `#`)
- Malformed string literals
- Invalid numeric literals

**Example error**:
```
[ERROR] Lexical Error at line 1, column 15: Unexpected character: '@'
```

### Phase 2: Syntax Analysis
**What it does**: Validates SQL grammar using recursive descent parser, builds AST

**Errors detected**:
- Missing keywords (e.g., missing FROM in SELECT)
- Incorrect operator usage
- Mismatched parentheses
- Invalid statement structure

**Example error**:
```
[ERROR] Syntax Error at line 1, column 20: Expected FROM keyword
```

### Phase 3: Semantic Analysis
**What it does**: Validates semantic correctness including types, table existence, column references

**Errors detected**:
- Table does not exist
- Column does not exist in table
- Type mismatches
- Ambiguous column references in JOINs

**Example error**:
```
[ERROR] Semantic Error at line 1, column 25: Table 'users' is not defined
```

### Phase 4: Intermediate Code Generation
**What it does**: Generates MongoDB JavaScript code from validated AST

**Errors detected**:
- Failed code generation
- Unsupported feature combinations

**Output**: MongoDB aggregation pipeline code

## Example Transformations

### SELECT with WHERE
```sql
SELECT * FROM users WHERE id = 1;
```

**Transforms to:**
```javascript
// MongoDB Aggregation Pipeline
db.users.aggregate([
  {
    "$match": { "$eq": ["$id", 1] }
  }
])
```

### SELECT with GROUP BY and Aggregation
```sql
SELECT user_id, COUNT(*) as order_count, SUM(amount) as total
FROM orders
GROUP BY user_id
HAVING COUNT(*) > 5
ORDER BY total DESC
LIMIT 10;
```

**Transforms to:**
```javascript
db.orders.aggregate([
  {
    "$group": {
      "_id": "$user_id",
      "order_count": { "$sum": 1 },
      "total": { "$sum": "$amount" }
    }
  },
  {
    "$sort": { "total": -1 }
  },
  {
    "$limit": 10
  }
])
```

### INSERT
```sql
INSERT INTO users (id, name, email)
VALUES (1, 'John', 'john@example.com');
```

**Transforms to:**
```javascript
db.users.insertOne({
  "id": 1,
  "name": "John",
  "email": "john@example.com"
})
```

### UPDATE
```sql
UPDATE users SET name = 'Jane' WHERE id = 1;
```

**Transforms to:**
```javascript
db.users.updateMany(
  { "$eq": ["$id", 1] },
  {
    "$set": {
      "name": "Jane"
    }
  }
)
```

### DELETE
```sql
DELETE FROM users WHERE id = 1;
```

**Transforms to:**
```javascript
db.users.deleteMany(
  { "$eq": ["$id", 1] }
)
```

## Compiler Architecture

```
┌─────────────────────────────────────────────────────────┐
│                    SQL Input                            │
└────────────────────┬────────────────────────────────────┘
                     │
                     ▼
         ┌───────────────────────┐
         │  Lexer                │
         │  (Tokenization)       │ ◄─ Collects LEXICAL_ERROR
         └────────────┬──────────┘
                      │
                      ▼
       ┌──────────────────────────────┐
       │  Parser                      │
       │  (Syntax Analysis, AST)      │ ◄─ Collects SYNTAX_ERROR
       └────────────┬─────────────────┘
                    │
                    ▼
    ┌────────────────────────────────────┐
    │  Semantic Analyzer                 │
    │  (Type & Reference Validation)     │ ◄─ Collects SEMANTIC_ERROR
    └────────────┬───────────────────────┘
                 │
         ┌───────▼──────────┐
         │ Errors Found?    │
         └───────┬──────────┘
                 │
        ┌────────┴────────┐
        │ YES             │ NO
        ▼                 ▼
    ┌────────┐    ┌──────────────────┐
    │ Report │    │ ICG Generator    │
    │ Errors │    │ (Code Generation)│ ◄─ Collects TRANSLATION_ERROR
    │ & Stop │    └────────┬─────────┘
    └────────┘             │
                           ▼
                ┌──────────────────────┐
                │  MongoDB JavaScript  │
                └──────────────────────┘
```

## Directory Structure

```
sql-nosql-transcompiler/
├── include/              # Header files (.h)
│   ├── ast.h            # AST node definitions
│   ├── error_handler.h  # Error management
│   ├── icg_generator.h  # Code generation
│   ├── lexer.h          # Lexical analyzer
│   ├── parser.h         # Syntax analyzer
│   ├── semantic_analyzer.h
│   ├── symbol_table.h   # Symbol table & type system
│   └── token.h          # Token definitions
├── src/                 # Implementation files (.cpp)
│   ├── main.cpp         # CLI entry point
│   ├── ast.cpp
│   ├── error_handler.cpp
│   ├── icg_generator.cpp
│   ├── lexer.cpp
│   ├── parser.cpp
│   ├── semantic_analyzer.cpp
│   ├── symbol_table.cpp
│   └── token.cpp
├── tests/               # Test suite
│   ├── CMakeLists.txt
│   └── test_compiler.cpp
├── examples/            # Example SQL files
│   ├── select_simple.sql
│   ├── select_complex.sql
│   ├── insert_example.sql
│   └── update_delete_example.sql
├── CMakeLists.txt       # Build configuration
├── README.md            # This file
└── BUILD.md             # Detailed build instructions
```

## Error Reporting

All errors are categorized by level and severity:

### Error Levels
1. **LEXICAL_ERROR** - Invalid tokens
2. **SYNTAX_ERROR** - Grammar violations
3. **SEMANTIC_ERROR** - Type/reference issues
4. **TRANSLATION_ERROR** - Code generation failures

### Error Severity
1. **WARNING** - Non-critical
2. **ERROR** - Critical (stops compilation)
3. **FATAL** - Catastrophic (immediate stop)

### Example Error Output
```
=== COMPILATION ERRORS AND WARNINGS ===

[ERROR] Semantic Error at line 1, column 20: Table 'invalid_table' is not defined
  Context: invalid_table

[ERROR] Syntax Error at line 2, column 15: Expected FROM keyword
  Context: SELECT id name

=== SUMMARY ===
Total Errors: 2
Total Warnings: 0
```

## Pre-registered Tables for Testing

The compiler comes with example tables:

```sql
-- Table: users
-- Columns: id (INT), name (STRING), email (STRING), age (INT), created_at (DATETIME)

-- Table: orders
-- Columns: id (INT), user_id (INT), total (FLOAT), status (STRING), created_at (DATETIME)

-- Table: products
-- Columns: id (INT), name (STRING), price (FLOAT), stock (INT)
```

Test queries against these tables:
```bash
$ ./transcompiler -i
SQL> SELECT * FROM users WHERE age > 25;
SQL> SELECT user_id, SUM(total) FROM orders GROUP BY user_id;
SQL> help
```

## Technical Details

### Implementation
- **Language**: C++17
- **Pattern**: Visitor pattern for AST traversal
- **Memory**: Smart pointers (shared_ptr, unique_ptr)
- **Data Structures**: 
  - unordered_map for symbol tables (O(1) lookup)
  - vector for error collection
  - Recursive descent parser

### Compilation Model
- **Error Recovery**: Parser uses synchronization points
- **Error Propagation**: All phases collect errors before proceeding
- **Type System**: Dynamic typing with basic type inference

## Building and Testing

```bash
# Build
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .

# Run tests
ctest

# Run compiler
./transcompiler --help
./transcompiler -f examples/select_complex.sql
./transcompiler -i
```

## Performance Notes

- Single-pass compilation
- O(n) lexical analysis
- O(n) parsing via recursive descent
- O(n) semantic analysis
- O(n) code generation
- Overall complexity: **O(n)** where n = input size

## Extension Guide

The modular design makes it easy to extend:

### Add New SQL Keywords
1. Update `token.h` - Add TokenType
2. Update `Lexer::keywords` in `lexer.cpp`
3. Update `Parser` for new grammar rules

### Support New NoSQL Target
1. Create `new_target_generator.h/cpp`
2. Inherit from ASTVisitor
3. Update main.cpp to use new generator

### Add Optimization Pass
1. Create visitor-based optimizer
2. Insert after semantic analysis
3. Return optimized AST

## Common Errors and Solutions

**Error: "Table 'xyz' is not defined"**
- Solution: Table name must be in the pre-registered tables or you need to register it

**Error: "Column 'abc' does not exist"**
- Solution: Check spelling and ensure column is defined in the table

**Error: "Expected FROM keyword"**
- Solution: SELECT statements require a FROM clause (or use VALUES clause)

**Error: "Unexpected character"**
- Solution: Remove or escape special characters not part of SQL

## Contributing

To contribute improvements:
1. Ensure code follows existing style
2. Add tests for new features
3. Update documentation
4. Test against pre-registered tables

## License

This project is provided as-is for educational purposes.

## Further Reading

- See `BUILD.md` for detailed build instructions
- See individual `.h` files for API documentation
- See `examples/` directory for sample queries

---

**Ready to compile SQL to MongoDB?**

Start with:
```bash
./transcompiler -i
```

Or run the test suite:
```bash
ctest
```

Happy compiling! 🎉

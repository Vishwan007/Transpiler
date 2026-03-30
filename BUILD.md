# SQL to NoSQL Transcompiler - Build Instructions

This is a comprehensive compiler for translating SQL queries to MongoDB JSON operations.

## Project Structure

```
├── include/                 # Header files
│   ├── ast.h              # Abstract Syntax Tree definitions
│   ├── error_handler.h    # Error management system
│   ├── icg_generator.h    # MongoDB code generator
│   ├── lexer.h            # Lexical analyzer
│   ├── parser.h           # Syntax analyzer
│   ├── semantic_analyzer.h # Semantic validation
│   ├── symbol_table.h     # Symbol table & type system
│   └── token.h            # Token definitions
├── src/                   # Implementation files
│   ├── main.cpp           # CLI tool entry point
│   ├── lexer.cpp          # Lexer implementation
│   ├── parser.cpp         # Parser implementation
│   ├── semantic_analyzer.cpp
│   ├── icg_generator.cpp  # MongoDB code generator
│   ├── error_handler.cpp
│   ├── symbol_table.cpp
│   ├── ast.cpp
│   └── token.cpp
├── tests/                 # Test suite
│   ├── test_compiler.cpp
│   └── CMakeLists.txt
├── examples/              # Example SQL queries
├── CMakeLists.txt         # Build configuration
└── README.md             # Documentation
```

## Building the Compiler

### Prerequisites

- C++17 or later compiler (GCC 8+, Clang 6+, MSVC 2017+)
- CMake 3.10 or later

### Build Steps

1. **Create a build directory:**
   ```bash
   mkdir build
   cd build
   ```

2. **Run CMake:**
   ```bash
   cmake ..
   ```

3. **Build the project:**
   ```bash
   cmake --build . --config Release
   ```

4. **Run tests (optional):**
   ```bash
   ctest
   ```

### Building on Linux/macOS

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make
./transcompiler --help
```

### Building on Windows

```bash
mkdir build
cd build
cmake ..
cmake --build . --config Release
Release\transcompiler.exe --help
```

## Running the Compiler

### Interactive Mode

```bash
./transcompiler -i
```

In interactive mode, you can:
- Type SQL queries and press Enter to compile
- Type `help` to see supported SQL statements
- Type `exit` or `quit` to exit

### File Input/Output Mode

```bash
./transcompiler -f input.sql -o output.mongodb
```

### Standard Input

```bash
./transcompiler -f query.sql
```

## Compiler Architecture

The compiler uses a 4-phase architecture with comprehensive error handling at each phase:

### Phase 1: Lexical Analysis (Lexer)
- Tokenizes SQL input into tokens
- Detects: invalid characters, malformed strings, unexpected symbols
- **Output**: Token stream
- **Errors Collected**: LEXICAL_ERROR

### Phase 2: Syntax Analysis (Parser)
- Builds Abstract Syntax Tree (AST) from token stream
- Validates SQL grammar and structure
- Detects: missing keywords, incorrect operators, mismatched parentheses
- **Output**: AST (or null if errors)
- **Errors Collected**: SYNTAX_ERROR

### Phase 3: Semantic Analysis (Semantic Analyzer)
- Validates semantic correctness
- Type checking, table/column existence validation
- Detects: undefined tables, undefined columns, type mismatches, JOIN errors
- **Output**: Validated AST
- **Errors Collected**: SEMANTIC_ERROR

### Phase 4: Intermediate Code Generation (ICG Generator)
- Generates MongoDB JSON queries from validated AST
- Converts SQL aggregation pipelines to MongoDB stages
- **Output**: MongoDB JavaScript code
- **Errors Collected**: TRANSLATION_ERROR

## Supported SQL Features

### SELECT Statements
- Simple selects: `SELECT * FROM table`
- Column selection: `SELECT col1, col2 FROM table`
- WHERE clause: `WHERE condition`
- GROUP BY with aggregates: `GROUP BY col` with `COUNT()`, `SUM()`, `AVG()`, `MIN()`, `MAX()`
- HAVING clause: `HAVING aggregate_condition`
- ORDER BY: `ORDER BY col ASC/DESC`
- LIMIT: `LIMIT n`
- DISTINCT: `SELECT DISTINCT col`

### JOIN Operations
- INNER JOIN: `JOIN table ON condition`
- LEFT JOIN: `LEFT JOIN table ON condition`
- RIGHT JOIN: `RIGHT JOIN table ON condition`
- FULL OUTER JOIN: `FULL OUTER JOIN table ON condition`

### INSERT Statements
- Single and multiple row inserts
- Column specification: `INSERT INTO table (cols) VALUES (vals)`

### UPDATE Statements
- Column updates: `UPDATE table SET col=val WHERE condition`
- Multiple assignments: `SET col1=val1, col2=val2`

### DELETE Statements
- Conditional deletion: `DELETE FROM table WHERE condition`

### Aggregate Functions
- COUNT(*), COUNT(column)
- SUM(column)
- AVG(column)
- MIN(column)
- MAX(column)

### Operators
- Comparison: `=`, `!=`, `<`, `>`, `<=`, `>=`
- Logical: `AND`, `OR`, `NOT`
- LIKE pattern matching
- BETWEEN range checks
- IN list membership
- IS NULL checks

## Error Handling

The compiler collects all errors at each compilation phase before proceeding. This allows you to:
- See all lexical errors before parsing
- See all syntax errors before semantic analysis
- See all semantic errors before code generation
- Understand exactly what needs to be fixed

### Error Levels
1. **LEXICAL_ERROR**: Invalid tokens or characters
2. **SYNTAX_ERROR**: Grammar violations
3. **SEMANTIC_ERROR**: Type mismatches, undefined references
4. **TRANSLATION_ERROR**: Code generation failures

### Error Severity
1. **WARNING**: Non-critical issues
2. **ERROR**: Critical, prevents compilation
3. **FATAL**: Catastrophic, stops compilation

Each error includes:
- Error type and severity
- Line and column number
- Descriptive message
- Code context (the problematic line)

Example error output:
```
[ERROR] Semantic Error at line 1, column 25: Column 'id' does not exist in table 'invalid_table'
  Context: invalid_table.id
```

## Pre-registered Tables

For testing, the compiler includes these pre-registered tables:

1. **users**
   - id (INT)
   - name (STRING)
   - email (STRING)
   - age (INT)
   - created_at (DATETIME)

2. **orders**
   - id (INT)
   - user_id (INT)
   - total (FLOAT)
   - status (STRING)
   - created_at (DATETIME)

3. **products**
   - id (INT)
   - name (STRING)
   - price (FLOAT)
   - stock (INT)

## Example Compilation Session

```
$ ./transcompiler -f examples/select_simple.sql

SQL to NoSQL Transcompiler
==========================
Input file: examples/select_simple.sql

[PHASE 1] Lexical Analysis...
  Tokens generated: 6

[PHASE 2] Syntax Analysis (Parsing)...
  AST successfully created

[PHASE 3] Semantic Analysis...
  Semantic validation passed

[PHASE 4] Code Generation (MongoDB)...
  MongoDB code successfully generated

MongoDB Output:
===============
// MongoDB Aggregation Pipeline
db.users.aggregate([
  {
    "$project": {
      "_id": 1
    }
  }
])
```

## Testing

Run the test suite:
```bash
cd build
ctest
```

Or run tests directly:
```bash
./test_compiler
```

The test suite includes:
- Lexer tests (tokenization)
- Parser tests (AST generation)
- Full compilation tests (end-to-end)

## Extension Points

The modular architecture makes it easy to extend:

1. **Add new SQL features**: Extend Parser and AST definitions
2. **Add new operators**: Update Lexer keywords and Parser expression handling
3. **Target new NoSQL databases**: Create new ICG generator classes
4. **Add optimization passes**: Insert between Semantic Analysis and ICG
5. **Support subqueries**: Extend Parser expression handling

## Performance Considerations

- The compiler uses shared_ptr for memory management
- Error handling uses std::vector for linear storage
- Symbol table uses std::unordered_map for O(1) lookups
- No dynamic memory is leaked (RAII principles)

## Limitations and Future Work

Current limitations:
- No subquery support
- No window functions
- No stored procedures
- No view definitions
- Limited SQL dialect support

Future enhancements:
- Support for more SQL dialects
- Optimization passes (constant folding, dead code elimination)
- Query plan visualization
- Support for other NoSQL databases (Cassandra, DynamoDB, etc.)
- REPL with query history
- Configuration file support for custom table definitions

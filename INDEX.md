# SQL to NoSQL Transcompiler - Documentation Index

Welcome! This document helps you navigate the project documentation.

## Quick Navigation

**Just want to get started?** → See [QUICK_REFERENCE.md](QUICK_REFERENCE.md) (5 min read)

**Need to build it?** → See [BUILD.md](BUILD.md) (10 min read)

**Want to understand how it works?** → See [COMPILER_ARCHITECTURE.md](COMPILER_ARCHITECTURE.md) (20 min read)

**Looking for features overview?** → See [README.md](README.md) (15 min read)

**Want project stats?** → See [PROJECT_SUMMARY.md](PROJECT_SUMMARY.md) (10 min read)

## Documentation Files

### [README.md](README.md) - Main Documentation (453 lines)
**Start here for a complete overview**
- Project overview and key features
- Supported SQL operations
- Quick start guide (build and run)
- Example SQL→MongoDB transformations
- Error reporting and categories
- Pre-registered tables
- Compiler architecture diagram
- Common issues and solutions

**Best for**: Understanding what the project does and what it supports

### [BUILD.md](BUILD.md) - Build Instructions (322 lines)
**Detailed building and running instructions**
- Project structure explanation
- Build prerequisites and steps
- Running the compiler (interactive and file modes)
- Compiler architecture in detail
- Supported SQL features list
- Pre-registered test tables
- Error handling guide
- Extension points for customization

**Best for**: Setting up the project, building, and running

### [QUICK_REFERENCE.md](QUICK_REFERENCE.md) - Quick Start Guide (309 lines)
**Fast reference for common tasks**
- 30-second build and run
- Compiler phases at a glance
- Supported SQL features table
- Interactive commands
- File mode examples
- Common transformations
- Error types
- Directory structure
- Test queries
- Common issues and solutions
- Tips and tricks

**Best for**: Quick lookup, quick start, common tasks

### [COMPILER_ARCHITECTURE.md](COMPILER_ARCHITECTURE.md) - Technical Design (568 lines)
**Deep dive into compiler internals**
- Complete compiler pipeline explanation
- Detailed component descriptions:
  - Lexer with algorithm details
  - Parser with grammar rules
  - Symbol table implementation
  - Semantic analyzer
  - ICG generator with MongoDB mappings
  - Error handler architecture
- Visitor pattern explanation
- Memory management approach
- Complexity analysis
- Design patterns used
- Extension points
- Performance optimizations

**Best for**: Understanding the code, contributing, extending

### [PROJECT_SUMMARY.md](PROJECT_SUMMARY.md) - Project Overview (425 lines)
**Statistics and summary of what was built**
- Project completion status
- Code structure and statistics
- Total lines and file counts
- Feature implementation status
- Supported SQL features
- MongoDB output examples
- Design patterns used
- Technical highlights
- Testing coverage
- Key accomplishments
- Future enhancement opportunities

**Best for**: Project overview, understanding scope

## Documentation by Topic

### Getting Started
1. Read [README.md](README.md) - Overview
2. Follow [BUILD.md](BUILD.md) - Build instructions
3. Use [QUICK_REFERENCE.md](QUICK_REFERENCE.md) - Run examples

### Using the Compiler
1. [QUICK_REFERENCE.md](QUICK_REFERENCE.md) - Commands and options
2. [README.md](README.md) - Examples and transformations
3. [BUILD.md](BUILD.md) - Detailed usage guide

### Understanding the Code
1. [COMPILER_ARCHITECTURE.md](COMPILER_ARCHITECTURE.md) - Architecture overview
2. Header files in `include/` - API documentation
3. Source files in `src/` - Implementation details

### Extending the Compiler
1. [COMPILER_ARCHITECTURE.md](COMPILER_ARCHITECTURE.md) - Extension points
2. [BUILD.md](BUILD.md) - Extension guide
3. Header files - API reference

### Testing
1. [BUILD.md](BUILD.md) - Test running instructions
2. `tests/test_compiler.cpp` - Test source code
3. `examples/` - Example SQL queries

## Code Structure

### Headers (include/)
```
├── ast.h              # Abstract Syntax Tree definitions
├── token.h            # Token types and utilities
├── lexer.h            # Lexical analyzer interface
├── parser.h           # Parser interface
├── semantic_analyzer.h # Semantic validator interface
├── symbol_table.h     # Symbol table and type system
├── icg_generator.h    # MongoDB code generator interface
└── error_handler.h    # Error collection and reporting
```

### Implementation (src/)
```
├── main.cpp               # CLI tool entry point
├── token.cpp              # Token utilities
├── lexer.cpp              # Lexical analyzer (309 lines)
├── parser.cpp             # Parser implementation (860 lines)
├── semantic_analyzer.cpp  # Semantic validation (293 lines)
├── symbol_table.cpp       # Symbol table (153 lines)
├── icg_generator.cpp      # MongoDB code generator (470 lines)
├── error_handler.cpp      # Error management (133 lines)
└── ast.cpp                # AST visitor implementations
```

### Tests & Examples
```
├── tests/
│   ├── test_compiler.cpp  # Test suite
│   └── CMakeLists.txt     # Test configuration
└── examples/
    ├── select_simple.sql       # Basic SELECT
    ├── select_complex.sql      # Complex aggregation
    ├── insert_example.sql      # INSERT statements
    └── update_delete_example.sql # UPDATE and DELETE
```

## Key Concepts

### The 4 Compiler Phases

1. **Lexical Analysis (Lexer)**
   - Location: `include/lexer.h`, `src/lexer.cpp`
   - Purpose: Break SQL into tokens
   - See: [QUICK_REFERENCE.md](QUICK_REFERENCE.md#compiler-phases-at-a-glance)

2. **Syntax Analysis (Parser)**
   - Location: `include/parser.h`, `src/parser.cpp`
   - Purpose: Build Abstract Syntax Tree (AST)
   - See: [COMPILER_ARCHITECTURE.md](COMPILER_ARCHITECTURE.md#2-parser-syntax-analysis)

3. **Semantic Analysis**
   - Location: `include/semantic_analyzer.h`, `src/semantic_analyzer.cpp`
   - Purpose: Validate types, tables, columns
   - See: [COMPILER_ARCHITECTURE.md](COMPILER_ARCHITECTURE.md#4-semantic-analyzer)

4. **Code Generation (ICG)**
   - Location: `include/icg_generator.h`, `src/icg_generator.cpp`
   - Purpose: Generate MongoDB JavaScript
   - See: [COMPILER_ARCHITECTURE.md](COMPILER_ARCHITECTURE.md#5-intermediate-code-generator)

### Error Handling
- **All Phases**: Errors collected before proceeding
- **Error Levels**: LEXICAL, SYNTAX, SEMANTIC, TRANSLATION
- **Error Severity**: WARNING, ERROR, FATAL
- **See**: [README.md](README.md#error-reporting) and [BUILD.md](BUILD.md#error-handling)

### Symbol Table
- **Purpose**: Track tables, columns, and types
- **Pre-registered**: users, orders, products
- **See**: [COMPILER_ARCHITECTURE.md](COMPILER_ARCHITECTURE.md#3-symbol-table--type-system)

## Common Tasks

### Build the Project
```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .
```
→ See [BUILD.md](BUILD.md#building-the-compiler)

### Run in Interactive Mode
```bash
./transcompiler -i
```
→ See [QUICK_REFERENCE.md](QUICK_REFERENCE.md#interactive-commands)

### Compile a SQL File
```bash
./transcompiler -f query.sql -o output.mongodb
```
→ See [QUICK_REFERENCE.md](QUICK_REFERENCE.md#file-mode-examples)

### Run Tests
```bash
cd build && ctest
```
→ See [BUILD.md](BUILD.md#testing)

### Add New SQL Feature
→ See [COMPILER_ARCHITECTURE.md](COMPILER_ARCHITECTURE.md#extension-points)

### Support New Database Target
→ See [COMPILER_ARCHITECTURE.md](COMPILER_ARCHITECTURE.md#adding-new-target-database)

## Quick Facts

- **Language**: C++17
- **Total Code**: 3,633 lines
- **Total Docs**: 2,177 lines
- **Phases**: 4 (Lexer, Parser, Semantic, ICG)
- **Error Handling**: Comprehensive, multi-level
- **Complexity**: O(n) linear time
- **Features**: Full SQL subset to MongoDB

## File Statistics

| File | Lines | Purpose |
|------|-------|---------|
| README.md | 453 | Main overview and guide |
| BUILD.md | 322 | Build and usage instructions |
| COMPILER_ARCHITECTURE.md | 568 | Technical architecture and design |
| PROJECT_SUMMARY.md | 425 | Project statistics and overview |
| QUICK_REFERENCE.md | 309 | Quick lookup and examples |
| **Documentation Total** | **2,077** | |
| | | |
| parser.cpp | 860 | Syntax analysis implementation |
| icg_generator.cpp | 470 | MongoDB code generation |
| semantic_analyzer.cpp | 293 | Type and reference validation |
| error_handler.cpp | 133 | Error collection |
| lexer.cpp | 309 | Tokenization |
| symbol_table.cpp | 153 | Symbol storage and lookup |
| main.cpp | 289 | CLI tool |
| **Source Total** | **2,507** | |
| | | |
| Header files | 532 | API definitions |
| Test suite | 170 | Unit and integration tests |
| **Total** | **5,286** | |

## Learning Path

### Level 1: User (30 minutes)
1. Read [QUICK_REFERENCE.md](QUICK_REFERENCE.md) (5 min)
2. Build the project (10 min)
3. Try examples (10 min)
4. Read relevant sections of [README.md](README.md) (5 min)

### Level 2: Developer (2 hours)
1. Read [README.md](README.md) (15 min)
2. Read [BUILD.md](BUILD.md) (20 min)
3. Build and test (20 min)
4. Read [COMPILER_ARCHITECTURE.md](COMPILER_ARCHITECTURE.md) (45 min)
5. Explore source code (20 min)

### Level 3: Contributor (4+ hours)
1. Complete Level 2 (2 hours)
2. Deep dive [COMPILER_ARCHITECTURE.md](COMPILER_ARCHITECTURE.md) (1 hour)
3. Study source files with documentation (1+ hours)
4. Understand each phase in detail

## Related Files in Project

### Example SQL Files
- `examples/select_simple.sql` - Basic SELECT
- `examples/select_complex.sql` - Complex aggregation
- `examples/insert_example.sql` - INSERT operations
- `examples/update_delete_example.sql` - UPDATE and DELETE

### Header Files (API Reference)
- `include/ast.h` - AST node definitions
- `include/parser.h` - Parser API
- `include/lexer.h` - Lexer API
- `include/semantic_analyzer.h` - Semantic analyzer API
- `include/icg_generator.h` - Code generator API
- `include/error_handler.h` - Error API
- `include/symbol_table.h` - Symbol table API
- `include/token.h` - Token API

## Support & Help

### For Usage Questions
→ See [QUICK_REFERENCE.md](QUICK_REFERENCE.md#common-issues--solutions)

### For Build Issues
→ See [BUILD.md](BUILD.md#troubleshooting)

### For Understanding Design
→ See [COMPILER_ARCHITECTURE.md](COMPILER_ARCHITECTURE.md)

### For Feature Information
→ See [README.md](README.md#supported-sql-operations)

### For Examples
→ See `examples/` directory or [QUICK_REFERENCE.md](QUICK_REFERENCE.md#test-queries)

## Document Map

```
INDEX.md (you are here)
├── README.md ..................... Main overview
├── QUICK_REFERENCE.md ............ Quick start & lookup
├── BUILD.md ...................... Build & usage instructions
├── COMPILER_ARCHITECTURE.md ...... Technical design details
├── PROJECT_SUMMARY.md ............ Project statistics
│
├── include/ ...................... Header files (API)
├── src/ .......................... Source code (implementation)
├── tests/ ........................ Test suite
├── examples/ ..................... Sample SQL queries
└── CMakeLists.txt ................ Build configuration
```

---

**Start here based on your needs:**

- **👤 User/Tester** → [QUICK_REFERENCE.md](QUICK_REFERENCE.md)
- **👨‍💻 Developer** → [README.md](README.md) + [BUILD.md](BUILD.md)
- **🏗️ Architect** → [COMPILER_ARCHITECTURE.md](COMPILER_ARCHITECTURE.md)
- **📊 Manager** → [PROJECT_SUMMARY.md](PROJECT_SUMMARY.md)

**Happy compiling!** 🎉

# SQL to NoSQL Transcompiler

A modern web-based compiler that transforms SQL queries into MongoDB aggregation pipelines with **comprehensive 4-phase error reporting**.

## Architecture Overview

The transcompiler implements the classic compiler pipeline with all errors collected **before intermediate code generation (ICG)**:

### Phase 1: Lexical Analysis
**Tokenization with Position Tracking**
- Character-by-character scanning with line/column precision
- Keyword recognition, string/number parsing
- Generates tokens with metadata

**Errors Collected:** `LEXICAL_ERROR` for invalid characters, malformed literals

### Phase 2: Syntax Analysis (Parser)
**Recursive Descent Parser**
- Builds Abstract Syntax Tree (AST) from token stream
- Validates SQL grammar without semantic checking
- Supports SELECT, INSERT, UPDATE, DELETE with clauses

**Errors Collected:** `SYNTAX_ERROR` for grammar violations, missing keywords, mismatched punctuation

### Phase 3: Semantic Analysis
**Type & Table Validation**
- Symbol table with table/column registry
- Validates table existence
- Validates column names and types
- Cross-references JOIN conditions

**Errors Collected:** `SEMANTIC_ERROR` for undefined tables/columns, type mismatches

### Phase 4: Intermediate Code Generation (ICG)
**MongoDB Code Generation**
- Only executes if all prior phases are error-free
- Generates aggregation pipelines
- Produces JavaScript-compatible MongoDB syntax

**Errors Collected:** `TRANSLATION_ERROR` if code generation fails

## Supported SQL Features

### Query Types
- ✅ `SELECT` with WHERE, GROUP BY, HAVING, ORDER BY, LIMIT, DISTINCT
- ✅ `INSERT INTO` with VALUES
- ✅ `UPDATE` with WHERE clause
- ✅ `DELETE` with WHERE clause
- ✅ `TRANSACTIONS` (BEGIN, COMMIT, ROLLBACK)

### JOINs
- ✅ INNER JOIN
- ✅ LEFT JOIN
- ✅ RIGHT JOIN
- ✅ FULL OUTER JOIN (planned)

### Aggregates
- ✅ COUNT(*)
- ✅ SUM(field)
- ✅ AVG(field)
- ✅ MIN(field)
- ✅ MAX(field)

### Operators
- ✅ Comparison: =, >, <, >=, <=, !=, <>
- ✅ Logical: AND, OR, NOT
- ✅ String: LIKE, IN, BETWEEN
- ✅ NULL checks: IS NULL, IS NOT NULL

### Clauses
- ✅ WHERE with conditions
- ✅ GROUP BY with HAVING
- ✅ ORDER BY (ASC/DESC)
- ✅ LIMIT & OFFSET

## Example Transformations

### Simple SELECT
**Input SQL:**
```sql
SELECT * FROM users WHERE age > 25 LIMIT 10;
```

**Output MongoDB:**
```javascript
db.users.aggregate([
  { "$match": { "age": { "$gt": 25 } } },
  { "$limit": 10 }
])
```

### Aggregation Query
**Input SQL:**
```sql
SELECT user_id, COUNT(*) as cnt, SUM(amount) as total
FROM orders
WHERE amount > 100
GROUP BY user_id
ORDER BY total DESC
LIMIT 10;
```

**Output MongoDB:**
```javascript
db.orders.aggregate([
  { "$match": { "amount": { "$gt": 100 } } },
  { "$group": { 
    "_id": "$user_id", 
    "cnt": { "$sum": 1 }, 
    "total": { "$sum": "$amount" } 
  } },
  { "$sort": { "total": -1 } },
  { "$limit": 10 },
  { "$project": { "user_id": 1, "cnt": 1, "total": 1 } }
])
```

### INSERT Operation
**Input SQL:**
```sql
INSERT INTO users VALUES ('John', 'john@example.com', 30);
```

**Output MongoDB:**
```javascript
db.users.insertOne({
  "field_0": "John",
  "field_1": "john@example.com",
  "field_2": 30
})
```

## Pre-registered Tables

The semantic analyzer includes validation for these default tables:

```
users: [id, name, email, age, created_at]
orders: [id, user_id, product_id, amount, created_at]
products: [id, name, price, stock]
customers: [id, name, phone, address]
```

## Error Handling Strategy

All errors from all phases are collected before ICG generation:

1. **Lexical errors stop immediately** - Cannot continue parsing invalid tokens
2. **Syntax errors accumulate** - Parser collects multiple grammar violations
3. **Semantic errors accumulate** - Analyzer checks all references
4. **Only on success** - ICG runs if all phases pass

**Example Error Display:**
```
LEXICAL_ERROR: Invalid character: '&' at Line 1, Column 15
SYNTAX_ERROR: Expected identifier, got 'WHERE' at Line 1, Column 8
SEMANTIC_ERROR: Table 'users' does not exist
Context: Available tables: users, orders, products, customers
```

## Project Structure

```
lib/
├── transcompiler.ts       # 749 lines - Complete 4-phase compiler
  ├── Lexer class
  ├── Parser class  
  ├── SemanticAnalyzer class
  ├── ICGGenerator class
  └── SQLToNoSQLTranscompiler (orchestrator)

app/
├── layout.tsx            # Root layout
├── page.tsx              # Main transcompiler interface (301 lines)
└── globals.css           # Dark theme with emerald accents

public/
└── [assets]

package.json
tsconfig.json
next.config.mjs
```

## Running the Application

### Development
```bash
npm run dev
# Opens at http://localhost:3000
```

### Production Build
```bash
npm run build
npm start
```

### Testing
1. Use quick examples: SELECT Simple, SELECT + Aggregate, INSERT, UPDATE
2. View errors in the "Errors" tab
3. Copy MongoDB output to clipboard

## Technology Stack

- **Frontend:** Next.js 15+ with React 19
- **Language:** TypeScript 5.x
- **Styling:** Tailwind CSS v4 with custom theme
- **Icons:** Lucide React
- **Compiler:** Custom TypeScript implementation

## Design Philosophy

1. **Clarity Over Complexity** - Clear error messages with context
2. **All Errors Upfront** - No surprises during code generation
3. **Extensible** - Easy to add SQL features or NoSQL targets
4. **User-Friendly** - Interactive web interface with examples

## Future Enhancements

- [ ] Support for subqueries
- [ ] CASE/WHEN expressions
- [ ] Window functions (OVER, PARTITION BY)
- [ ] CTE (Common Table Expressions)
- [ ] Additional NoSQL targets (Firestore, DynamoDB, Redis)
- [ ] Query optimization suggestions
- [ ] Real-time syntax highlighting
- [ ] Query execution in MongoDB Atlas

## Building for Production

Deploy to Vercel or any Node.js hosting:

```bash
# Build
npm run build

# Deploy
vercel deploy
```

## Performance

- **Lexical Analysis:** O(n) where n = input length
- **Parsing:** O(n) recursive descent
- **Semantic Analysis:** O(n) single pass with symbol table lookup
- **Total:** O(n) across all phases

Typical query compilation: <10ms

## License

MIT - Open source for educational purposes

---

**Built with ❤️ as a Compiler Design Project**

This transcompiler demonstrates core compiler concepts: lexical analysis, syntax parsing, semantic validation, and code generation. Perfect for learning compiler architecture patterns.

#include <iostream>
#include <cassert>
#include <memory>
#include "lexer.h"
#include "parser.h"
#include "semantic_analyzer.h"
#include "icg_generator.h"
#include "symbol_table.h"
#include "error_handler.h"

class CompilerTestSuite {
private:
    std::shared_ptr<ErrorHandler> errorHandler;
    std::shared_ptr<SymbolTable> symbolTable;
    
    void setupTestDatabase() {
        symbolTable->registerTable("users",
                                 {"id", "name", "email"},
                                 {DataType::INT, DataType::STRING, DataType::STRING});
        
        symbolTable->registerTable("orders",
                                 {"id", "user_id", "amount"},
                                 {DataType::INT, DataType::INT, DataType::FLOAT});
    }
    
    bool testLexer(const std::string& input, const std::string& testName) {
        std::cout << "Testing Lexer: " << testName << "... ";
        
        auto errHandler = std::make_shared<ErrorHandler>();
        Lexer lexer(input, errHandler);
        auto tokens = lexer.tokenize();
        
        if (errHandler->hasErrors()) {
            std::cout << "FAILED (Lexical Errors)\n";
            errHandler->printErrors(std::cout);
            return false;
        }
        
        std::cout << "PASSED (" << tokens.size() << " tokens)\n";
        return true;
    }
    
    bool testParser(const std::string& input, const std::string& testName) {
        std::cout << "Testing Parser: " << testName << "... ";
        
        auto errHandler = std::make_shared<ErrorHandler>();
        Lexer lexer(input, errHandler);
        auto tokens = lexer.tokenize();
        
        Parser parser(tokens, errHandler);
        auto ast = parser.parse();
        
        if (errHandler->hasErrors()) {
            std::cout << "FAILED (Syntax Errors)\n";
            errHandler->printErrors(std::cout);
            return false;
        }
        
        if (!ast) {
            std::cout << "FAILED (No AST generated)\n";
            return false;
        }
        
        std::cout << "PASSED\n";
        return true;
    }
    
    bool testFullCompilation(const std::string& input, const std::string& testName) {
        std::cout << "Testing Full Compilation: " << testName << "... ";
        
        auto errHandler = std::make_shared<ErrorHandler>();
        
        // Lexical Analysis
        Lexer lexer(input, errHandler);
        auto tokens = lexer.tokenize();
        
        if (errHandler->hasCriticalErrors()) {
            std::cout << "FAILED (Lexical errors)\n";
            return false;
        }
        
        // Syntax Analysis
        Parser parser(tokens, errHandler);
        auto ast = parser.parse();
        
        if (errHandler->hasCriticalErrors() || !ast) {
            std::cout << "FAILED (Syntax errors)\n";
            return false;
        }
        
        // Semantic Analysis
        SemanticAnalyzer semanticAnalyzer(symbolTable, errHandler);
        if (auto selectStmt = std::dynamic_pointer_cast<SelectStatement>(ast)) {
            semanticAnalyzer.analyzeSelectStatement(selectStmt);
        } else if (auto insertStmt = std::dynamic_pointer_cast<InsertStatement>(ast)) {
            semanticAnalyzer.analyzeInsertStatement(insertStmt);
        }
        
        if (errHandler->hasCriticalErrors()) {
            std::cout << "FAILED (Semantic errors)\n";
            return false;
        }
        
        // Code Generation
        ICGGenerator icgGenerator(errHandler);
        std::string code;
        if (auto selectStmt = std::dynamic_pointer_cast<SelectStatement>(ast)) {
            code = icgGenerator.generateForSelect(selectStmt);
        } else if (auto insertStmt = std::dynamic_pointer_cast<InsertStatement>(ast)) {
            code = icgGenerator.generateForInsert(insertStmt);
        }
        
        if (errHandler->hasErrors() || code.empty()) {
            std::cout << "FAILED (Code generation failed)\n";
            return false;
        }
        
        std::cout << "PASSED\n";
        return true;
    }
    
public:
    CompilerTestSuite() : errorHandler(std::make_shared<ErrorHandler>()),
                         symbolTable(std::make_shared<SymbolTable>()) {
        setupTestDatabase();
    }
    
    int runAllTests() {
        int passed = 0;
        int failed = 0;
        
        std::cout << "\n=== COMPILER TEST SUITE ===\n\n";
        
        // Lexer Tests
        std::cout << "--- LEXER TESTS ---\n";
        if (testLexer("SELECT * FROM users", "Simple SELECT")) passed++; else failed++;
        if (testLexer("SELECT id, name FROM users WHERE id = 1", "SELECT with WHERE")) passed++; else failed++;
        if (testLexer("INSERT INTO users VALUES (1, 'John', 'john@example.com')", "INSERT")) passed++; else failed++;
        if (testLexer("UPDATE users SET name = 'Jane' WHERE id = 1", "UPDATE")) passed++; else failed++;
        if (testLexer("DELETE FROM users WHERE id = 1", "DELETE")) passed++; else failed++;
        
        std::cout << "\n--- PARSER TESTS ---\n";
        if (testParser("SELECT * FROM users", "Simple SELECT")) passed++; else failed++;
        if (testParser("SELECT id, name FROM users WHERE id = 1", "SELECT with WHERE")) passed++; else failed++;
        if (testParser("SELECT id, COUNT(*) FROM users GROUP BY id", "SELECT with GROUP BY")) passed++; else failed++;
        if (testParser("SELECT * FROM users ORDER BY id ASC", "SELECT with ORDER BY")) passed++; else failed++;
        if (testParser("INSERT INTO users VALUES (1, 'John', 'john@example.com')", "INSERT")) passed++; else failed++;
        if (testParser("UPDATE users SET name = 'Jane' WHERE id = 1", "UPDATE")) passed++; else failed++;
        if (testParser("DELETE FROM users WHERE id = 1", "DELETE")) passed++; else failed++;
        
        std::cout << "\n--- FULL COMPILATION TESTS ---\n";
        if (testFullCompilation("SELECT * FROM users", "Simple SELECT")) passed++; else failed++;
        if (testFullCompilation("SELECT id, name FROM users WHERE id = 1", "SELECT with WHERE")) passed++; else failed++;
        if (testFullCompilation("SELECT id, COUNT(*) FROM users GROUP BY id", "SELECT with GROUP BY")) passed++; else failed++;
        if (testFullCompilation("INSERT INTO users VALUES (1, 'John', 'john@example.com')", "INSERT")) passed++; else failed++;
        
        std::cout << "\n=== TEST SUMMARY ===\n";
        std::cout << "Passed: " << passed << "\n";
        std::cout << "Failed: " << failed << "\n";
        std::cout << "Total:  " << (passed + failed) << "\n\n";
        
        return failed == 0 ? 0 : 1;
    }
};

int main() {
    CompilerTestSuite suite;
    return suite.runAllTests();
}

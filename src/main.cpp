#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <memory>
#include <filesystem>
#include "lexer.h"
#include "parser.h"
#include "semantic_analyzer.h"
#include "icg_generator.h"
#include "symbol_table.h"
#include "error_handler.h"

namespace fs = std::filesystem;

class SQLToNoSQLCompiler {
private:
    std::shared_ptr<ErrorHandler> errorHandler;
    std::shared_ptr<SymbolTable> symbolTable;
    
public:
    SQLToNoSQLCompiler() : errorHandler(std::make_shared<ErrorHandler>()),
                           symbolTable(std::make_shared<SymbolTable>()) {}
    
    bool initializeDatabase() {
        // Pre-register some example tables for testing
        symbolTable->registerTable("users", 
                                 {"id", "name", "email", "age", "created_at"},
                                 {DataType::INT, DataType::STRING, DataType::STRING, 
                                  DataType::INT, DataType::DATETIME});
        
        symbolTable->registerTable("orders",
                                 {"id", "user_id", "total", "status", "created_at"},
                                 {DataType::INT, DataType::INT, DataType::FLOAT,
                                  DataType::STRING, DataType::DATETIME});
        
        symbolTable->registerTable("products",
                                 {"id", "name", "price", "stock"},
                                 {DataType::INT, DataType::STRING, DataType::FLOAT, DataType::INT});
        
        return true;
    }
    
    std::string compile(const std::string& sqlQuery) {
        errorHandler->clearErrors();
        
        // Phase 1: Lexical Analysis
        std::cout << "[PHASE 1] Lexical Analysis...\n";
        Lexer lexer(sqlQuery, errorHandler);
        auto tokens = lexer.tokenize();
        
        if (errorHandler->hasCriticalErrors()) {
            std::cout << "Lexical errors found. Displaying errors before proceeding...\n\n";
            errorHandler->printErrors(std::cout);
            return "";
        }
        
        // Debug: print tokens
        std::cout << "  Tokens generated: " << tokens.size() << "\n\n";
        
        // Phase 2: Syntax Analysis (Parsing)
        std::cout << "[PHASE 2] Syntax Analysis (Parsing)...\n";
        Parser parser(tokens, errorHandler);
        auto ast = parser.parse();
        
        if (errorHandler->hasCriticalErrors()) {
            std::cout << "Syntax errors found. Displaying errors before proceeding...\n\n";
            errorHandler->printErrors(std::cout);
            return "";
        }
        
        if (!ast) {
            std::cout << "Failed to create AST. Aborting compilation.\n\n";
            errorHandler->printErrors(std::cout);
            return "";
        }
        
        std::cout << "  AST successfully created\n\n";
        
        // Phase 3: Semantic Analysis
        std::cout << "[PHASE 3] Semantic Analysis...\n";
        SemanticAnalyzer semanticAnalyzer(symbolTable, errorHandler);
        
        // Analyze based on statement type
        if (auto selectStmt = std::dynamic_pointer_cast<SelectStatement>(ast)) {
            semanticAnalyzer.analyzeSelectStatement(selectStmt);
        } else if (auto insertStmt = std::dynamic_pointer_cast<InsertStatement>(ast)) {
            semanticAnalyzer.analyzeInsertStatement(insertStmt);
        } else if (auto updateStmt = std::dynamic_pointer_cast<UpdateStatement>(ast)) {
            semanticAnalyzer.analyzeUpdateStatement(updateStmt);
        } else if (auto deleteStmt = std::dynamic_pointer_cast<DeleteStatement>(ast)) {
            semanticAnalyzer.analyzeDeleteStatement(deleteStmt);
        }
        
        if (errorHandler->hasCriticalErrors()) {
            std::cout << "Semantic errors found. Displaying errors before code generation...\n\n";
            errorHandler->printErrors(std::cout);
            return "";
        }
        
        std::cout << "  Semantic validation passed\n\n";
        
        // Phase 4: Intermediate Code Generation (MongoDB)
        std::cout << "[PHASE 4] Code Generation (MongoDB)...\n";
        ICGGenerator icgGenerator(errorHandler);
        std::string mongodbCode;
        
        if (auto selectStmt = std::dynamic_pointer_cast<SelectStatement>(ast)) {
            mongodbCode = icgGenerator.generateForSelect(selectStmt);
        } else if (auto insertStmt = std::dynamic_pointer_cast<InsertStatement>(ast)) {
            mongodbCode = icgGenerator.generateForInsert(insertStmt);
        } else if (auto updateStmt = std::dynamic_pointer_cast<UpdateStatement>(ast)) {
            mongodbCode = icgGenerator.generateForUpdate(updateStmt);
        } else if (auto deleteStmt = std::dynamic_pointer_cast<DeleteStatement>(ast)) {
            mongodbCode = icgGenerator.generateForDelete(deleteStmt);
        } else if (auto txnStmt = std::dynamic_pointer_cast<TransactionStatement>(ast)) {
            mongodbCode = icgGenerator.generateForTransaction(txnStmt);
        }
        
        if (errorHandler->hasErrors()) {
            std::cout << "Code generation errors:\n\n";
            errorHandler->printErrors(std::cout);
            return "";
        }
        
        std::cout << "  MongoDB code successfully generated\n\n";
        
        return mongodbCode;
    }
    
    const std::shared_ptr<ErrorHandler>& getErrorHandler() const {
        return errorHandler;
    }
};

void printUsage(const char* programName) {
    std::cout << "SQL to NoSQL (MongoDB) Transcompiler\n";
    std::cout << "====================================\n\n";
    std::cout << "Usage: " << programName << " [options]\n\n";
    std::cout << "Options:\n";
    std::cout << "  -f, --file <path>        Read SQL from file\n";
    std::cout << "  -o, --output <path>      Write MongoDB output to file\n";
    std::cout << "  -i, --interactive        Interactive mode (read from stdin)\n";
    std::cout << "  -v, --verbose            Verbose output with detailed error info\n";
    std::cout << "  -h, --help               Show this help message\n\n";
    std::cout << "Examples:\n";
    std::cout << "  " << programName << " -f query.sql -o output.mongodb\n";
    std::cout << "  " << programName << " -i\n";
    std::cout << "  " << programName << " --file input.sql --verbose\n\n";
}

int main(int argc, char* argv[]) {
    std::string inputFile;
    std::string outputFile;
    bool interactiveMode = false;
    bool verboseMode = false;
    
    // Parse command-line arguments
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        
        if (arg == "-h" || arg == "--help") {
            printUsage(argv[0]);
            return 0;
        }
        else if ((arg == "-f" || arg == "--file") && i + 1 < argc) {
            inputFile = argv[++i];
        }
        else if ((arg == "-o" || arg == "--output") && i + 1 < argc) {
            outputFile = argv[++i];
        }
        else if (arg == "-i" || arg == "--interactive") {
            interactiveMode = true;
        }
        else if (arg == "-v" || arg == "--verbose") {
            verboseMode = true;
        }
        else {
            std::cerr << "Unknown option: " << arg << "\n";
            printUsage(argv[0]);
            return 1;
        }
    }
    
    // Initialize compiler
    SQLToNoSQLCompiler compiler;
    compiler.initializeDatabase();
    
    // Interactive mode
    if (interactiveMode) {
        std::cout << "SQL to NoSQL Transcompiler - Interactive Mode\n";
        std::cout << "============================================\n";
        std::cout << "Enter SQL queries (type 'exit' to quit, 'help' for help)\n\n";
        
        std::string line;
        while (true) {
            std::cout << "SQL> ";
            std::getline(std::cin, line);
            
            if (line == "exit" || line == "quit") {
                std::cout << "Exiting...\n";
                break;
            }
            
            if (line == "help") {
                std::cout << "Supported SQL statements:\n";
                std::cout << "  SELECT ... FROM ... WHERE ... GROUP BY ... ORDER BY ... LIMIT\n";
                std::cout << "  INSERT INTO ... VALUES ...\n";
                std::cout << "  UPDATE ... SET ... WHERE ...\n";
                std::cout << "  DELETE FROM ... WHERE ...\n";
                std::cout << "  BEGIN TRANSACTION / COMMIT / ROLLBACK\n\n";
                continue;
            }
            
            if (line.empty()) {
                continue;
            }
            
            std::cout << "\n";
            std::string result = compiler.compile(line);
            
            if (!result.empty()) {
                std::cout << "MongoDB Output:\n";
                std::cout << "===============\n";
                std::cout << result << "\n\n";
            }
        }
        
        return 0;
    }
    
    // File mode
    if (inputFile.empty()) {
        std::cerr << "Error: Input file not specified. Use -f/--file or -i/--interactive\n";
        printUsage(argv[0]);
        return 1;
    }
    
    // Read input file
    std::ifstream inFile(inputFile);
    if (!inFile.is_open()) {
        std::cerr << "Error: Cannot open input file: " << inputFile << "\n";
        return 1;
    }
    
    std::stringstream buffer;
    buffer << inFile.rdbuf();
    std::string sqlQuery = buffer.str();
    inFile.close();
    
    std::cout << "SQL to NoSQL Transcompiler\n";
    std::cout << "==========================\n";
    std::cout << "Input file: " << inputFile << "\n";
    if (!outputFile.empty()) {
        std::cout << "Output file: " << outputFile << "\n";
    }
    std::cout << "\n";
    
    // Compile
    std::string result = compiler.compile(sqlQuery);
    
    // Output result
    if (!result.empty()) {
        if (outputFile.empty()) {
            // Print to stdout
            std::cout << "MongoDB Output:\n";
            std::cout << "===============\n";
            std::cout << result << "\n";
        } else {
            // Write to file
            std::ofstream outFile(outputFile);
            if (!outFile.is_open()) {
                std::cerr << "Error: Cannot create output file: " << outputFile << "\n";
                return 1;
            }
            
            outFile << result;
            outFile.close();
            
            std::cout << "MongoDB code successfully written to: " << outputFile << "\n";
        }
        
        return 0;
    } else {
        std::cout << "Compilation failed.\n";
        return 1;
    }
}

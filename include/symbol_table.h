#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include <string>
#include <unordered_map>
#include <vector>
#include <memory>

enum class SymbolType {
    TABLE,
    COLUMN,
    VARIABLE,
    FUNCTION,
    AGGREGATE
};

enum class DataType {
    INT,
    FLOAT,
    STRING,
    BOOLEAN,
    DATETIME,
    JSON,
    UNKNOWN
};

struct Symbol {
    std::string name;
    SymbolType type;
    DataType dataType;
    std::string tableName;  // For columns, which table they belong to
    bool isNullable;
    int lineNumber;
    int columnNumber;
    
    Symbol(const std::string& n, SymbolType t, DataType dt, int line = 0, int col = 0)
        : name(n), type(t), dataType(dt), isNullable(true), 
          lineNumber(line), columnNumber(col) {}
};

class Scope {
private:
    std::unordered_map<std::string, std::shared_ptr<Symbol>> symbols;
    std::shared_ptr<Scope> parentScope;
    std::string scopeName;
    
public:
    Scope(const std::string& name, std::shared_ptr<Scope> parent = nullptr)
        : scopeName(name), parentScope(parent) {}
    
    void addSymbol(const std::shared_ptr<Symbol>& symbol);
    std::shared_ptr<Symbol> lookup(const std::string& name);
    std::shared_ptr<Symbol> lookupLocal(const std::string& name);
    bool exists(const std::string& name);
    void removeSymbol(const std::string& name);
    
    std::shared_ptr<Scope> getParent() const { return parentScope; }
    std::string getName() const { return scopeName; }
    const auto& getSymbols() const { return symbols; }
};

class SymbolTable {
private:
    std::shared_ptr<Scope> currentScope;
    std::shared_ptr<Scope> globalScope;
    std::unordered_map<std::string, std::vector<std::string>> tableColumns;  // table -> columns
    std::unordered_map<std::string, std::vector<DataType>> tableColumnTypes;  // table -> column types
    
public:
    SymbolTable();
    
    // Scope management
    void enterScope(const std::string& scopeName);
    void exitScope();
    std::shared_ptr<Scope> getCurrentScope() const { return currentScope; }
    std::shared_ptr<Scope> getGlobalScope() const { return globalScope; }
    
    // Symbol operations
    void addSymbol(const std::shared_ptr<Symbol>& symbol);
    std::shared_ptr<Symbol> lookup(const std::string& name);
    std::shared_ptr<Symbol> lookupLocal(const std::string& name);
    bool exists(const std::string& name);
    
    // Table operations
    void registerTable(const std::string& tableName, 
                      const std::vector<std::string>& columnNames,
                      const std::vector<DataType>& columnTypes);
    bool isValidTable(const std::string& tableName) const;
    bool isValidColumn(const std::string& tableName, const std::string& columnName) const;
    std::vector<std::string> getTableColumns(const std::string& tableName) const;
    DataType getColumnType(const std::string& tableName, const std::string& columnName) const;
    
    // Utility
    void clear();
    void printSymbolTable(std::ostream& out = std::cout) const;
};

#endif // SYMBOL_TABLE_H

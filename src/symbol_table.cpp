#include "symbol_table.h"
#include <iostream>

void Scope::addSymbol(const std::shared_ptr<Symbol>& symbol) {
    symbols[symbol->name] = symbol;
}

std::shared_ptr<Symbol> Scope::lookup(const std::string& name) {
    auto it = symbols.find(name);
    if (it != symbols.end()) {
        return it->second;
    }
    if (parentScope) {
        return parentScope->lookup(name);
    }
    return nullptr;
}

std::shared_ptr<Symbol> Scope::lookupLocal(const std::string& name) {
    auto it = symbols.find(name);
    if (it != symbols.end()) {
        return it->second;
    }
    return nullptr;
}

bool Scope::exists(const std::string& name) {
    return lookup(name) != nullptr;
}

void Scope::removeSymbol(const std::string& name) {
    symbols.erase(name);
}

SymbolTable::SymbolTable() {
    globalScope = std::make_shared<Scope>("global", nullptr);
    currentScope = globalScope;
    
    // Pre-register built-in aggregate functions
    auto countFunc = std::make_shared<Symbol>("COUNT", SymbolType::AGGREGATE, DataType::INT);
    auto sumFunc = std::make_shared<Symbol>("SUM", SymbolType::AGGREGATE, DataType::FLOAT);
    auto avgFunc = std::make_shared<Symbol>("AVG", SymbolType::AGGREGATE, DataType::FLOAT);
    auto minFunc = std::make_shared<Symbol>("MIN", SymbolType::AGGREGATE, DataType::UNKNOWN);
    auto maxFunc = std::make_shared<Symbol>("MAX", SymbolType::AGGREGATE, DataType::UNKNOWN);
    
    globalScope->addSymbol(countFunc);
    globalScope->addSymbol(sumFunc);
    globalScope->addSymbol(avgFunc);
    globalScope->addSymbol(minFunc);
    globalScope->addSymbol(maxFunc);
}

void SymbolTable::enterScope(const std::string& scopeName) {
    auto newScope = std::make_shared<Scope>(scopeName, currentScope);
    currentScope = newScope;
}

void SymbolTable::exitScope() {
    if (currentScope->getParent()) {
        currentScope = currentScope->getParent();
    }
}

void SymbolTable::addSymbol(const std::shared_ptr<Symbol>& symbol) {
    currentScope->addSymbol(symbol);
}

std::shared_ptr<Symbol> SymbolTable::lookup(const std::string& name) {
    return currentScope->lookup(name);
}

std::shared_ptr<Symbol> SymbolTable::lookupLocal(const std::string& name) {
    return currentScope->lookupLocal(name);
}

bool SymbolTable::exists(const std::string& name) {
    return currentScope->exists(name);
}

void SymbolTable::registerTable(const std::string& tableName,
                               const std::vector<std::string>& columnNames,
                               const std::vector<DataType>& columnTypes) {
    tableColumns[tableName] = columnNames;
    tableColumnTypes[tableName] = columnTypes;
    
    auto tableSymbol = std::make_shared<Symbol>(tableName, SymbolType::TABLE, DataType::UNKNOWN);
    globalScope->addSymbol(tableSymbol);
}

bool SymbolTable::isValidTable(const std::string& tableName) const {
    return tableColumns.find(tableName) != tableColumns.end();
}

bool SymbolTable::isValidColumn(const std::string& tableName, const std::string& columnName) const {
    auto it = tableColumns.find(tableName);
    if (it == tableColumns.end()) {
        return false;
    }
    const auto& columns = it->second;
    return std::find(columns.begin(), columns.end(), columnName) != columns.end();
}

std::vector<std::string> SymbolTable::getTableColumns(const std::string& tableName) const {
    auto it = tableColumns.find(tableName);
    if (it != tableColumns.end()) {
        return it->second;
    }
    return {};
}

DataType SymbolTable::getColumnType(const std::string& tableName, const std::string& columnName) const {
    auto it = tableColumnTypes.find(tableName);
    if (it == tableColumnTypes.end()) {
        return DataType::UNKNOWN;
    }
    
    auto colIt = tableColumns.find(tableName);
    if (colIt == tableColumns.end()) {
        return DataType::UNKNOWN;
    }
    
    const auto& columns = colIt->second;
    const auto& types = it->second;
    
    for (size_t i = 0; i < columns.size(); i++) {
        if (columns[i] == columnName && i < types.size()) {
            return types[i];
        }
    }
    return DataType::UNKNOWN;
}

void SymbolTable::clear() {
    tableColumns.clear();
    tableColumnTypes.clear();
    globalScope = std::make_shared<Scope>("global", nullptr);
    currentScope = globalScope;
}

void SymbolTable::printSymbolTable(std::ostream& out) const {
    out << "=== SYMBOL TABLE ===\n\n";
    out << "Tables:\n";
    for (const auto& [tableName, columns] : tableColumns) {
        out << "  " << tableName << " (";
        for (size_t i = 0; i < columns.size(); i++) {
            if (i > 0) out << ", ";
            out << columns[i];
        }
        out << ")\n";
    }
    out << "\n";
}

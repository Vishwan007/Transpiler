#ifndef ERROR_HANDLER_H
#define ERROR_HANDLER_H

#include <string>
#include <vector>
#include <iostream>
#include <sstream>

enum class ErrorLevel {
    LEXICAL_ERROR,      // Invalid token or character
    SYNTAX_ERROR,       // Invalid grammar
    SEMANTIC_ERROR,     // Invalid type or reference
    TRANSLATION_ERROR   // ICG generation error
};

enum class ErrorSeverity {
    WARNING,   // Non-critical, may affect execution
    ERROR,     // Critical, prevents compilation
    FATAL      // Catastrophic, stops compilation
};

struct CompileError {
    ErrorLevel level;
    ErrorSeverity severity;
    int line;
    int column;
    std::string message;
    std::string context;  // The problematic code snippet

    CompileError(ErrorLevel lvl, ErrorSeverity sev, int l, int c, 
                 const std::string& msg, const std::string& ctx = "")
        : level(lvl), severity(sev), line(l), column(c), 
          message(msg), context(ctx) {}

    std::string toString() const;
};

class ErrorHandler {
private:
    std::vector<CompileError> errors;
    bool stopOnError;

public:
    ErrorHandler(bool stopOnFirstError = false) : stopOnError(stopOnFirstError) {}

    void addError(ErrorLevel level, ErrorSeverity severity, int line, int column,
                  const std::string& message, const std::string& context = "");

    void addLexicalError(int line, int column, const std::string& message,
                        const std::string& context = "");
    void addSyntaxError(int line, int column, const std::string& message,
                       const std::string& context = "");
    void addSemanticError(int line, int column, const std::string& message,
                         const std::string& context = "");
    void addTranslationError(int line, int column, const std::string& message,
                            const std::string& context = "");

    bool hasErrors() const;
    bool hasCriticalErrors() const;  // ERROR or FATAL severity
    int getErrorCount() const;
    int getWarningCount() const;

    const std::vector<CompileError>& getErrors() const { return errors; }
    
    void printErrors(std::ostream& out = std::cerr) const;
    std::string getErrorSummary() const;
    
    void clearErrors() { errors.clear(); }
    
    // Check if should continue compilation
    bool canContinue() const;
};

#endif // ERROR_HANDLER_H

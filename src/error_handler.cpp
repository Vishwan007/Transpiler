#include "error_handler.h"
#include <iomanip>

std::string CompileError::toString() const {
    std::stringstream ss;
    
    std::string levelStr;
    switch (level) {
        case ErrorLevel::LEXICAL_ERROR:    levelStr = "Lexical Error";    break;
        case ErrorLevel::SYNTAX_ERROR:     levelStr = "Syntax Error";     break;
        case ErrorLevel::SEMANTIC_ERROR:   levelStr = "Semantic Error";   break;
        case ErrorLevel::TRANSLATION_ERROR: levelStr = "Translation Error"; break;
    }
    
    std::string sevStr;
    switch (severity) {
        case ErrorSeverity::WARNING: sevStr = "[WARNING]"; break;
        case ErrorSeverity::ERROR:   sevStr = "[ERROR]";   break;
        case ErrorSeverity::FATAL:   sevStr = "[FATAL]";   break;
    }
    
    ss << sevStr << " " << levelStr << " at line " << line << ", column " << column << ": "
       << message;
    if (!context.empty()) {
        ss << "\n  Context: " << context;
    }
    
    return ss.str();
}

void ErrorHandler::addError(ErrorLevel level, ErrorSeverity severity, int line, int column,
                           const std::string& message, const std::string& context) {
    errors.emplace_back(level, severity, line, column, message, context);
    
    if (stopOnError && severity != ErrorSeverity::WARNING) {
        // Could implement early exit here if needed
    }
}

void ErrorHandler::addLexicalError(int line, int column, const std::string& message,
                                  const std::string& context) {
    addError(ErrorLevel::LEXICAL_ERROR, ErrorSeverity::ERROR, line, column, message, context);
}

void ErrorHandler::addSyntaxError(int line, int column, const std::string& message,
                                 const std::string& context) {
    addError(ErrorLevel::SYNTAX_ERROR, ErrorSeverity::ERROR, line, column, message, context);
}

void ErrorHandler::addSemanticError(int line, int column, const std::string& message,
                                   const std::string& context) {
    addError(ErrorLevel::SEMANTIC_ERROR, ErrorSeverity::ERROR, line, column, message, context);
}

void ErrorHandler::addTranslationError(int line, int column, const std::string& message,
                                      const std::string& context) {
    addError(ErrorLevel::TRANSLATION_ERROR, ErrorSeverity::ERROR, line, column, message, context);
}

bool ErrorHandler::hasErrors() const {
    for (const auto& err : errors) {
        if (err.severity != ErrorSeverity::WARNING) {
            return true;
        }
    }
    return false;
}

bool ErrorHandler::hasCriticalErrors() const {
    for (const auto& err : errors) {
        if (err.severity == ErrorSeverity::ERROR || err.severity == ErrorSeverity::FATAL) {
            return true;
        }
    }
    return false;
}

int ErrorHandler::getErrorCount() const {
    int count = 0;
    for (const auto& err : errors) {
        if (err.severity != ErrorSeverity::WARNING) {
            count++;
        }
    }
    return count;
}

int ErrorHandler::getWarningCount() const {
    int count = 0;
    for (const auto& err : errors) {
        if (err.severity == ErrorSeverity::WARNING) {
            count++;
        }
    }
    return count;
}

void ErrorHandler::printErrors(std::ostream& out) const {
    if (errors.empty()) {
        out << "No errors found.\n";
        return;
    }
    
    // Sort errors by line number for better readability
    auto sortedErrors = errors;
    std::sort(sortedErrors.begin(), sortedErrors.end(),
              [](const CompileError& a, const CompileError& b) {
                  if (a.line != b.line) return a.line < b.line;
                  return a.column < b.column;
              });
    
    out << "\n=== COMPILATION ERRORS AND WARNINGS ===\n\n";
    
    for (const auto& err : sortedErrors) {
        out << err.toString() << "\n";
    }
    
    out << "\n=== SUMMARY ===\n";
    out << "Total Errors: " << getErrorCount() << "\n";
    out << "Total Warnings: " << getWarningCount() << "\n";
}

std::string ErrorHandler::getErrorSummary() const {
    std::stringstream ss;
    ss << "Errors: " << getErrorCount() << ", Warnings: " << getWarningCount();
    return ss.str();
}

bool ErrorHandler::canContinue() const {
    // Continue if only warnings, stop if there are errors or fatal errors
    return !hasCriticalErrors();
}

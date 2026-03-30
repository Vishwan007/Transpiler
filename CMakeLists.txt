cmake_minimum_required(VERSION 3.10)
project(SQLToNoSQLCompiler)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O2")

# Main executable
add_executable(transcompiler
    src/main.cpp
    src/lexer.cpp
    src/parser.cpp
    src/semantic_analyzer.cpp
    src/icg_generator.cpp
    src/error_handler.cpp
    src/symbol_table.cpp
)

# Include directories
target_include_directories(transcompiler PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Optional: Create a library for reuse
add_library(transcompiler_lib
    src/lexer.cpp
    src/parser.cpp
    src/semantic_analyzer.cpp
    src/icg_generator.cpp
    src/error_handler.cpp
    src/symbol_table.cpp
)

target_include_directories(transcompiler_lib PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Testing (optional)
enable_testing()
add_subdirectory(tests)

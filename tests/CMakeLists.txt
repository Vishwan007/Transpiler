add_executable(test_compiler
    test_compiler.cpp
)

target_include_directories(test_compiler PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)
target_link_libraries(test_compiler PRIVATE transcompiler_lib)

add_test(NAME CompilerTests COMMAND test_compiler)

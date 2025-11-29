cmake_minimum_required(VERSION 3.15)
project(ambra_lang LANGUAGES CXX)

# Set C++ standard to C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Source files for the core library
set(CORE_SOURCES
    src/lexer/lexer.cpp
    src/lexer/token/token.cpp
    src/parser/parser.cpp
    src/ast/ast.cpp
    src/sema/analyzer.cpp
    src/bytecode/emitter.cpp
    src/bytecode/disassembler.cpp
    src/vm/vm.cpp
    src/runtime/builtins.cpp
    src/utils/error.cpp
)

set(CORE_HEADERS
    src/lexer/lexer.h
    src/lexer/token/token.h
    src/parser/parser.h
    src/ast/ast.h
    src/sema/analyzer.h
    src/bytecode/bytecode.h
    src/bytecode/emitter.h
    src/vm/vm.h
    src/vm/value.h
    src/runtime/builtins.h
    src/utils/error.h
    src/utils/string_utils.h
)

# Create core library
add_library(ambra_core ${CORE_SOURCES} ${CORE_HEADERS})
target_include_directories(ambra_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

# Compiler executable
add_executable(ambra_compiler src/cli/compiler_main.cpp)
target_link_libraries(ambra_compiler ambra_core)

# VM executable
add_executable(ambra_vm src/cli/vm_main.cpp)
target_link_libraries(ambra_vm ambra_core)

# Tests
enable_testing()

add_executable(lexer_tests tests/lexer_tests.cpp)
target_link_libraries(lexer_tests ambra_core)
add_test(NAME LexerTests COMMAND lexer_tests)

add_executable(parser_tests tests/parser_tests.cpp)
target_link_libraries(parser_tests ambra_core)
add_test(NAME ParserTests COMMAND parser_tests)

add_executable(vm_tests tests/vm_tests.cpp)
target_link_libraries(vm_tests ambra_core)
add_test(NAME VMTests COMMAND vm_tests)

# Optional: Print build configuration
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "C++ Compiler: ${CMAKE_CXX_COMPILER}")

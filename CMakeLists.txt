cmake_minimum_required(VERSION 3.15)
project(ambra_lang LANGUAGES CXX)

# Set C++ standard to C++17
set(CMAKE_CXX_STANDARD 17)
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
    src/ir/lowering.cpp
    src/bytecode/emitter.cpp
    src/bytecode/disassembler.cpp
    src/vm/vm.cpp
    src/runtime/builtins.cpp
    src/utils/error.cpp
)

# Find clang-format
find_program(CLANG_FORMAT_EXECUTABLE NAMES clang-format)

if(CLANG_FORMAT_EXECUTABLE)
    message(STATUS "Found clang-format: ${CLANG_FORMAT_EXECUTABLE}")
    
    # Collect all source files to format
    file(GLOB_RECURSE ALL_SOURCE_FILES 
        ${CMAKE_SOURCE_DIR}/src/*.cpp 
        ${CMAKE_SOURCE_DIR}/src/*.h
        ${CMAKE_SOURCE_DIR}/tests/*.cpp
    )
    
    # Create a custom target that runs clang-format
    add_custom_target(format_code
        COMMAND ${CLANG_FORMAT_EXECUTABLE} -i -style=file ${ALL_SOURCE_FILES}
        COMMENT "Running clang-format on source files..."
        VERBATIM
    )
else()
    message(WARNING "clang-format not found. Auto-formatting will be skipped.")
endif()

# Create core library
add_library(ambra_lang ${CORE_SOURCES})
target_include_directories(ambra_lang PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

# Make formatting run before building the library
if(CLANG_FORMAT_EXECUTABLE)
    add_dependencies(ambra_lang format_code)
endif()

# Compiler executable
add_executable(ambra_compiler src/cli/compiler_main.cpp)
target_link_libraries(ambra_compiler ambra_lang)

# VM executable
add_executable(ambra_vm src/cli/vm_main.cpp)
target_link_libraries(ambra_vm ambra_lang)

# Google Test setup (must come before add_subdirectory(tests))
include(FetchContent)

FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.zip
  DOWNLOAD_EXTRACT_TIMESTAMP ON
)

FetchContent_MakeAvailable(googletest)

# For macOS: avoid overriding your project compiler flags
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

# Tests
enable_testing()
add_subdirectory(tests)

# Optional: Print build configuration
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "C++ Compiler: ${CMAKE_CXX_COMPILER}")
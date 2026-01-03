enable_testing()
include(GoogleTest)

# lexer_tests
add_executable(lexer_tests lexer_tests.cpp)
target_link_libraries(lexer_tests PRIVATE gtest gtest_main ambra_lang)
gtest_discover_tests(lexer_tests)

# parser_tests
add_executable(parser_tests parser_tests.cpp)
target_link_libraries(parser_tests PRIVATE gtest gtest_main ambra_lang)
gtest_discover_tests(parser_tests)

# resolver_tests
add_executable(analyzer_tests analyzer_tests.cpp)
target_link_libraries(analyzer_tests PRIVATE gtest gtest_main ambra_lang)
gtest_discover_tests(analyzer_tests)

# lowering_tests
add_executable(lowering_tests lowering_tests.cpp)
target_link_libraries(lowering_tests PRIVATE gtest gtest_main ambra_lang)
gtest_discover_tests(lowering_tests)

# vm_tests
add_executable(vm_tests vm_tests.cpp)
target_link_libraries(vm_tests PRIVATE gtest gtest_main ambra_lang)
gtest_discover_tests(vm_tests)
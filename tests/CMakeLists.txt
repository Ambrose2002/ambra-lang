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
add_executable(resolver_tests resolver_tests.cpp)
target_link_libraries(resolver_tests PRIVATE gtest gtest_main ambra_lang)
gtest_discover_tests(resolver_tests)

# vm_tests
add_executable(vm_tests vm_tests.cpp)
target_link_libraries(vm_tests PRIVATE gtest gtest_main ambra_lang)
gtest_discover_tests(vm_tests)
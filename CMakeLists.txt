cmake_minimum_required(VERSION 3.14)
project(sql_lexer CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

add_library(lexer_core STATIC
    src/lexer.cpp
    src/token.cpp
    src/input.cpp
)

add_executable(sql_lexer src/main.cpp)
target_link_libraries(sql_lexer PRIVATE lexer_core)
 
enable_testing()
add_executable(test_lexer tests/test_lexer.cpp)
target_link_libraries(test_lexer PRIVATE lexer_core)
add_test(NAME lexer_tests COMMAND test_lexer)

# ---------------- Practica 5: analizador sintactico LL(1) ----------------
add_library(parser_core STATIC
    parser_ll1/src/grammar.cpp
)
target_include_directories(parser_core PUBLIC parser_ll1/include)
target_link_libraries(parser_core PUBLIC lexer_core)

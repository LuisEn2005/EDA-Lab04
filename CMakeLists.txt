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
    parser_ll1/src/parser.cpp
    parser_ll1/src/parse_table.cpp
    parser_ll1/src/trace.cpp
)
target_include_directories(parser_core PUBLIC parser_ll1/include)
target_link_libraries(parser_core PUBLIC lexer_core)

add_executable(test_parser parser_ll1/tests/test_parser.cpp)
target_link_libraries(test_parser PRIVATE parser_core)
add_test(NAME parser_tests COMMAND test_parser)
# Ejecutable del parser (lexer + parser LL(1) + traza)
add_executable(parser_ll1 parser_ll1/src/main.cpp)
target_link_libraries(parser_ll1 PRIVATE parser_core)

# Pruebas de la tabla construida, la traza y las consultas invalidas
add_executable(test_parser_tabla parser_ll1/tests/test_tabla_errores.cpp)
target_link_libraries(test_parser_tabla PRIVATE parser_core)
add_test(NAME parser_tabla_tests COMMAND test_parser_tabla)

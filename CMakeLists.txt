cmake_minimum_required(VERSION 3.14)
project(sql_lexer CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

add_library(lexer_core STATIC
    src/lexer.cpp
    src/token.cpp
)

add_executable(sql_lexer src/main.cpp)
target_link_libraries(sql_lexer PRIVATE lexer_core)
 
enable_testing()
add_executable(test_lexer tests/test_lexer.cpp)
target_link_libraries(test_lexer PRIVATE lexer_core)
add_test(NAME lexer_tests COMMAND test_lexer)
 


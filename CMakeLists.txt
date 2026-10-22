cmake_minimum_required(VERSION 3.14)
project(sql_lexer CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

add_executable(sql_lexer
    src/Lexer.cpp
    src/main.cpp
)

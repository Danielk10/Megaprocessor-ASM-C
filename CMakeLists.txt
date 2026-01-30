cmake_minimum_required(VERSION 3.10)
project(megap_asm C)

set(CMAKE_C_STANDARD 99)

add_executable(megap-asm
    src/main.c
    src/lexer.c
    src/parser.c
    src/codegen.c
)

target_include_directories(megap-asm PRIVATE include)

cmake_minimum_required(VERSION 3.10)
project(megaprocessor_asm C)

set(CMAKE_C_STANDARD 99)

add_executable(megaprocessor-asm
    src/main.c
    src/lexer.c
    src/parser.c
    src/codegen.c
)

target_include_directories(megaprocessor-asm PRIVATE include)

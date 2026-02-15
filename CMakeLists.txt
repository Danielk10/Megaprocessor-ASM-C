cmake_minimum_required(VERSION 3.10)
project(megap_asm C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_executable(megap-asm
    src/main.c
    src/assembler.c
)

target_include_directories(megap-asm PRIVATE include)

install(TARGETS megap-asm
    RUNTIME DESTINATION bin
)

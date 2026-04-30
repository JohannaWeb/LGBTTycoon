cmake_minimum_required(VERSION 3.20)
project(lgbtycoon C ASM_NASM)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_ASM_NASM_OBJECT_FORMAT elf64)

find_package(SDL2 REQUIRED)

add_executable(lgbtycoon main.c game.c entity.c structure.c tick_decay_simd.asm render.c)
target_link_libraries(lgbtycoon PRIVATE SDL2::SDL2)

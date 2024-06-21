cmake_minimum_required(VERSION 3.0.0)
project(chip8 VERSION 0.1.0 LANGUAGES C)
find_package(SDL2 CONFIG REQUIRED)
    
add_executable(chip8 main.c)

target_link_libraries(chip8
    PRIVATE
    $<TARGET_NAME_IF_EXISTS:SDL2::SDL2main>
    $<IF:$<TARGET_EXISTS:SDL2::SDL2>,SDL2::SDL2,SDL2::SDL2-static>
)

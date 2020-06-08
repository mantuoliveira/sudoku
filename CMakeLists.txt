cmake_minimum_required(VERSION 3.16)
project(Sudoku)

set(CMAKE_CXX_STANDARD 17)

add_executable(Sudoku sudoku.cpp)
target_link_libraries(Sudoku SDL2 SDL2_ttf SDL2_gfx)
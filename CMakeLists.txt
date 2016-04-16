cmake_minimum_required(VERSION 3.2)
include(cmakego)
add_subdirectory(include/SOIL)
add_definitions(--std=c++11)
usepackage(assimp eigen glew glfw REQUIRED)
include_directories(include)
link_libraries(p::eigen p::glew p::glfw SOIL)
add_subdirectory(src)



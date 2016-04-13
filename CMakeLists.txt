include(cmakego)

add_definitions(--std=c++11)
usepackage(assimp eigen glew glfw REQUIRED)
include_directories(include)
link_libraries(p::eigen p::glew p::glfw)
add_subdirectory(src)



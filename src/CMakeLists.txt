
add_executable(minimal minimal.cpp)

add_executable(minimal2 minimal2.cpp)

add_executable(minimal3 minimal3.cpp)

add_executable(assimp1 assimp1.cpp)
target_link_libraries(assimp1 p::assimp)

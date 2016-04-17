
add_executable(minimal minimal.cpp)

add_executable(minimal2 minimal2.cpp)

add_executable(minimal3 minimal3.cpp)

add_executable(minimal4 minimal4.cpp)

add_executable(minimal5 minimal5.cpp)

add_executable(assimp1 assimp1.cpp)
target_link_libraries(assimp1 p::assimp)

add_executable(assimp1capture assimp1capture.cpp)
target_link_libraries(assimp1capture SOIL p::assimp)

#check if p::egl
#add_executable(X11less X11less.cpp)
#target_link_libraries(X11less EGL)


add_executable(minimal minimal.cpp)

add_executable(minimal2 minimal2.cpp)

add_executable(minimal3 minimal3.cpp)

add_executable(minimal4 minimal4.cpp)

add_executable(minimal5 minimal5.cpp)

add_executable(assimp1 assimp1.cpp)
target_link_libraries(assimp1 p::assimp)

add_executable(assimp1capture assimp1capture.cpp)
target_link_libraries(assimp1capture SOIL p::assimp)

#TBC
add_executable(assimp1deferred assimp1deferred.cpp)
target_link_libraries(assimp1deferred SOIL p::assimp)

#add_executable(assimp1oit assimp1oit.cpp)
#target_link_libraries(assimp1oit SOIL p::assimp)

#TBC
if(UTHREE)
add_library(uthree SHARED uthree.cpp)

add_executable(uthree1 uthree1.cpp)
target_link_libraries(uthree1 uthree)
endif()

#check if p::egl
if(TARGET p::egl)
add_executable(X11less X11less.cpp)
target_link_libraries(X11less EGL)
endif()

if(TARGET p::gstreamer)
add_executable(video1 video1.cpp)
target_link_libraries(video1 p::gstreamer)
endif()
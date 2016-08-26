
link_libraries(SOIL p::assimp)

#add_executable(minimal minimal.cpp)

#add_executable(minimal2 minimal2.cpp)

#add_executable(minimal3 minimal3.cpp)

#add_executable(minimal4 minimal4.cpp)

add_executable(simplequad simplequad.cpp)

add_executable(simplecube simplecube.cpp)

add_executable(aroverlay aroverlay.cpp)


#add_executable(raytrace1 raytrace1.cpp)

#add_executable(showimage showimage.cpp)

add_executable(assimp1 assimp1.cpp)
target_link_libraries(assimp1 p::assimp)

#add_executable(assimp1ar assimp1ar.cpp)
#target_link_libraries(assimp1ar p::assimp)

add_executable(assimp1capture assimp1capture.cpp)

add_executable(assimp1deferred assimp1deferred.cpp)

#TBC
#target_link_libraries(assimp1deferred SOIL p::assimp)

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

#add_executable(testarcamera testarcamera.cpp)

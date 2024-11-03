cmake_minimum_required(VERSION 3.29)
project(camera_app)

set(CMAKE_CXX_STANDARD 23)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wpedantic -Wextra")

find_package(glm REQUIRED)

add_executable(camera_app main.cpp
        gfx.cpp
        gfx.hpp
        camera.cpp
        camera.hpp)

target_link_libraries(camera_app PRIVATE glm)
target_include_directories(camera_app PRIVATE deps/include)
wut_create_rpx(camera_app)


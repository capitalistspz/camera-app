cmake_minimum_required(VERSION 3.29)
project(camera_app)

set(CMAKE_CXX_STANDARD 23)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wpedantic -Wextra")


add_executable(camera_app main.cpp
        graphics.cpp
        graphics.hpp
        camera.cpp
        camera.hpp)

target_include_directories(camera_app PRIVATE deps/include)
wut_create_rpx(camera_app)


cmake_minimum_required(VERSION 3.3)
project(Raytracer)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES template-rt.cpp)
add_executable(Raytracer ${SOURCE_FILES})

cmake_minimum_required(VERSION 3.13)

add_subdirectory(sfml)

project(forces)

set(CMAKE_CXX_FLAGS_RELEASE "-O3")

add_executable(forces main.cpp)

target_include_directories(forces SYSTEM PRIVATE sfml/include)
target_link_libraries(forces sfml-system sfml-graphics sfml-window)

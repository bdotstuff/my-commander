cmake_minimum_required(VERSION 3.31)
project(my_commander)

include(FetchContent)
set(CMAKE_CXX_STANDARD 20)
set(BUILD_SHARED_LIBS OFF)
FETCHCONTENT_DECLARE(SFML GIT_REPOSITORY https://github.com/SFML/SFML.git GIT_TAG 3.0.2)
FETCHCONTENT_MAKEAVAILABLE(SFML)

add_executable(my_commander main.cpp)

target_link_libraries(my_commander sfml-graphics)

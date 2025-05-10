cmake_minimum_required(VERSION 3.11 FATAL_ERROR)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project("KARATSUBA_MULTIPLICATION" VERSION 0.1.0 LANGUAGES CXX)

set(SOURCES 
"src/main.cpp"
"src/Karatsuba_multiplication.hpp"
"src/Karatsuba_multiplication.cpp"
"src/Binary_multiplication.hpp"
"src/Binary_multiplication.cpp"
"src/timer.hpp" 
"src/timer.cpp"
)

add_executable(${PROJECT_NAME} ${SOURCES})

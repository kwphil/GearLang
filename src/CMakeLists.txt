cmake_minimum_required(VERSION 3.20)

project(gearlang LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_C_STANDARD 90)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(SOURCES
    src/main.cpp
)

add_executable(gearlang ${SOURCES})

target_include_directories(gearlang PRIVATE include)


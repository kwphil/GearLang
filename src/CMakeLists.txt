cmake_minimum_required(VERSION 3.20)

project(gearlang LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_C_STANDARD 90)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

find_package(LLVM REQUIRED CONFIG)

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

set(SOURCES
    src/main.cpp
    src/ast.cpp
    src/lex.cpp
    src/ctx.cpp
)

add_executable(gearlang ${SOURCES})

llvm_map_components_to_libnames(LLVM_LIBS 
    core
    support
    irreader
)


target_include_directories(gearlang PRIVATE ${LLVM_LIBS})


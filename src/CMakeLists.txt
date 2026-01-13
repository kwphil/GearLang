cmake_minimum_required(VERSION 3.20)

project(gearlang LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_C_STANDARD 90)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(CMAKE_BUILD_TYPE Debug)

find_package(LLVM REQUIRED CONFIG)

add_executable(gearlang
    src/main.cpp
    src/ast.cpp
    src/lex.cpp
    src/ctx.cpp
)

target_include_directories(gearlang PRIVATE ${LLVM_INCLUDE_DIRS})
target_compile_definitions(gearlang PRIVATE ${LLVM_DEFINITIONS})

llvm_map_components_to_libnames(LLVM_LIBS
    core
    support
    irreader
)

target_link_libraries(gearlang PRIVATE LLVM)


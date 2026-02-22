cmake_minimum_required(VERSION 3.20)

project(gearlang LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_C_STANDARD 90)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(CMAKE_BUILD_TYPE Debug)

include(CheckCXXCompilerFlag)

function(enable_cxx_compiler_flag_if_supported flag)
    string(FIND "${CMAKE_CXX_FLAGS}" "${flag}" flag_already_set)
    if(flag_already_set EQUAL -1)
        check_cxx_compiler_flag("${flag}" flag_supported)
        if(flag_supported)
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${flag}" PARENT_SCOPE)
        endif()
        unset(flag_supported CACHE)
    endif()
endfunction()

# enable_cxx_compiler_flag_if_supported("-Wall")
# enable_cxx_compiler_flag_if_supported("-Wextra")

find_package(LLVM REQUIRED CONFIG)

add_executable(gearlang
    main.cpp
    lex.cpp
    ctx.cpp
    error.cpp
    fsm.cpp
    func.cpp
    startup.cpp
    sem/analyze.cpp
    sem/type.cpp
    analyze/decl.cpp
    analyze/literal.cpp
    analyze/ops.cpp
    generate/base.cpp
    generate/expr.cpp
    generate/literal.cpp
    generate/func.cpp
    parse/base.cpp
    parse/expr.cpp
    parse/func.cpp
)

target_include_directories(gearlang PRIVATE 
    ${LLVM_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

target_compile_definitions(gearlang PRIVATE ${LLVM_DEFINITIONS})

llvm_map_components_to_libnames(LLVM_LIBS
    core
    support
    irreader
)

target_link_libraries(gearlang PRIVATE LLVM)


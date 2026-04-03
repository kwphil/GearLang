#    _____                 _                       
#   / ____|               | |                      
#  | |  __  ___  __ _ _ __| |     __ _ _ __   __ _ 
#  | | |_ |/ _ \/ _` | '__| |    / _` | '_ \ / _` | Clean, Clear and Fast Code
#  | |__| |  __/ (_| | |  | |___| (_| | | | | (_| | https://github.com/kwphil/gearlang
#   \_____|\___|\__,_|_|  |______\__,_|_| |_|\__, |
#                                             __/ |
#                                            |___/ 
#
# Licensed under the MIT License <https://opensource.org/licenses/MIT>.
# SPDX-License-Identifier: MIT
#
# Permission is hereby  granted, free of charge, to any  person obtaining a copy
# of this software and associated  documentation files (the "Software"), to deal
# in the Software  without restriction, including without  limitation the rights
# to  use, copy,  modify, merge,  publish, distribute,  sublicense, and/or  sell
# copies  of  the Software,  and  to  permit persons  to  whom  the Software  is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE  IS PROVIDED "AS  IS", WITHOUT WARRANTY  OF ANY KIND,  EXPRESS OR
# IMPLIED,  INCLUDING BUT  NOT  LIMITED TO  THE  WARRANTIES OF  MERCHANTABILITY,
# FITNESS FOR  A PARTICULAR PURPOSE AND  NONINFRINGEMENT. IN NO EVENT  SHALL THE
# AUTHORS  OR COPYRIGHT  HOLDERS  BE  LIABLE FOR  ANY  CLAIM,  DAMAGES OR  OTHER
# LIABILITY, WHETHER IN AN ACTION OF  CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE  OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

cmake_minimum_required(VERSION 3.20)

if(NOT DEFINED COMPILE_MODE)
    set(COMPILE_MODE "executable")
endif()

project(gearlang LANGUAGES C CXX)

find_program(CCACHE_PROGRAM ccache)
if (CCACHE_PROGRAM)
    set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
    set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
    message(STATUS "Using CCache")
else()
    message(STATUS "Not using CCache, consider installing it for faster compilation")
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_C_STANDARD 90)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

if(DEBUG AND NOT NO_DEBUG)
    set(CMAKE_BUILD_TYPE Debug)
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
    include(CheckCXXCompilerFlag)
    set(DEBUG_FLAGS "-fsanitize=undefined,address -O0 -g -D_DEBUG")
    if(NOT DEFINED ABORT_ERROR)
        set(ABORT_ERROR ON)
    endif()
endif()

if(ABORT_ERROR)
    set_source_files_properties(
        error.cpp
        PROPERTIES COMPILE_DEFINITIONS "ABORT_ON_FAIL"
    )
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -Wall ${DEBUG_FLAGS}")

find_package(LLVM REQUIRED CONFIG)
find_package(Clang REQUIRED CONFIG)

llvm_map_components_to_libnames(llvm_libs support core irreader)


set(GEARLANG_CORE
    error.cpp
    func.cpp
    setup.cpp
    optimize.cpp 
)

set(GEARLANG_SEM
    sem/analyze.cpp
    sem/type/core.cpp
    sem/type/llvm.cpp
    sem/type/parse.cpp
)

set(GEARLANG_ANALYZE
    ana/vars.cpp
    ana/literal.cpp
    ana/stmt.cpp
    ana/ops.cpp
    ana/expr.cpp
)

add_subdirectory(par ${CMAKE_BINARY_DIR}/CMakeFiles/par)
add_subdirectory(gen ${CMAKE_BINARY_DIR}/CMakeFiles/gen)
add_subdirectory(lex ${CMAKE_BINARY_DIR}/CMakeFiles/lex)
add_subdirectory(ffi ${CMAKE_BINARY_DIR}/CMakeFiles/ffi)

add_library(gearlib SHARED
    ${GEARLANG_CORE}
    ${GEARLANG_LEX}
    ${GEARLANG_SEM}
    ${GEARLANG_ANALYZE}
)

add_executable(gear main.cpp)

target_link_libraries(gearlib
    gear_parse
    gear_gen
    gear_lex 
    gear_ffi_c
    LLVM
    ${Clang_LIBRARIES}
    clang
    clang-cpp
)

target_link_libraries(gear gearlib)

target_include_directories(gearlib SYSTEM PUBLIC
    ${LLVM_INCLUDE_DIRS}
)
target_include_directories(gearlib PUBLIC
    ${CMAKE_SOURCE_DIR}/../include
)

target_include_directories(gear PRIVATE
    ${CMAKE_SOURCE_DIR}/../include
    ${LLVM_INCLUDE_DIRS}
)

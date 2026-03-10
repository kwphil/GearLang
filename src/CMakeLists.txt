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

if( NOT DEFINED COMPILE_MODE )
    set(COMPILE_MODE "executable")
endif() 

project(gearlang LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
include(CheckCXXCompilerFlag)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -Wall -fsanitize=address")

set(CMAKE_C_STANDARD 90)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(LLVM REQUIRED CONFIG)

set(GEARLANG_CORE
    error.cpp
    func.cpp
    setup.cpp
)

set(GEARLANG_LEX
    lex/char.cpp
    lex/classify.cpp
    lex/tokenize.cpp
    lex/lex.cpp
)

set(GEARLANG_SEM
    sem/analyze.cpp
    sem/type/core.cpp
    sem/type/llvm.cpp
    sem/type/parse.cpp
)

set(GEARLANG_ANALYZE
    analyze/vars.cpp
    analyze/literal.cpp
    analyze/stmt.cpp
    analyze/ops.cpp
    analyze/expr.cpp
)

add_subdirectory(parse)
add_subdirectory(generate)

add_library(libgear SHARED
    ${GEARLANG_CORE}
    ${GEARLANG_LEX}
    ${GEARLANG_SEM}
    ${GEARLANG_ANALYZE}
    ${GEARLANG_GENERATE}
    ${GEARLANG_PARSE}
)

add_executable(gear main.cpp)
target_link_libraries(libgear gear_parse gear_gen LLVM)
target_link_libraries(gear libgear)

target_include_directories(libgear PRIVATE 
    ${LLVM_INCLUDE_DIRS} 
    ${CMAKE_SOURCE_DIR}/../include
)

target_include_directories(gear PRIVATE ${CMAKE_SOURCE_DIR}/../include ${LLVM_INCLUDE_DIRS})

llvm_map_components_to_libnames(LLVM_LIBS
    core
    support
    irreader
)


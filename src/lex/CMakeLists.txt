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

# find_program(RE2C_EXECUTABLE re2c REQUIRED)

# set(LEXER_RE ${CMAKE_SOURCE_DIR}/lex/tokenize.cpp)
# set(LEXER_CPP re2c.cpp)

# add_custom_command(
#     OUTPUT ${LEXER_CPP}
#     COMMAND ${RE2C_EXECUTABLE}
#             -o ${LEXER_CPP}
#             ${LEXER_RE}
#     DEPENDS ${LEXER_RE}
#     COMMENT "Generating lexer with re2c"
# )

set(GEARLANG_LEX
    char.cpp
    classify.cpp
    lex.cpp
    tokenize.cpp
    ${LEXER_CPP}
)

add_library(gear_lex STATIC ${GEARLANG_LEX}) 
target_include_directories(gear_lex PRIVATE 
    ${LLVM_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/../include
)

set_target_properties(gear_lex PROPERTIES
    OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/CMakeFiles/gear_lex.dir"
)

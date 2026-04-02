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

set(GEARLANG_FFI_C
    c_lang/core.cpp
    c_lang/visitor.cpp
    c_lang/front.cpp
)

add_library(gear_ffi_c STATIC ${GEARLANG_FFI_C}) 
target_include_directories(gear_ffi_c PRIVATE 
    ${LLVM_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/../include
)

# Note that ffi_c will be required
set_target_properties(gear_ffi_c PROPERTIES
    OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/CMakeFiles/gear_ffi_c.dir"
) 
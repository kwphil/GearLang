/*
   _____                 _                       
  / ____|               | |                      
 | |  __  ___  __ _ _ __| |     __ _ _ __   __ _ 
 | | |_ |/ _ \/ _` | '__| |    / _` | '_ \ / _` | Clean, Clear and Fast Code
 | |__| |  __/ (_| | |  | |___| (_| | | | | (_| | https://github.com/kwphil/gearlang
  \_____|\___|\__,_|_|  |______\__,_|_| |_|\__, |
                                            __/ |
                                           |___/ 

Licensed under the MIT License <https://opensource.org/licenses/MIT>.
SPDX-License-Identifier: MIT

Permission is hereby  granted, free of charge, to any  person obtaining a copy
of this software and associated  documentation files (the "Software"), to deal
in the Software  without restriction, including without  limitation the rights
to  use, copy,  modify, merge,  publish, distribute,  sublicense, and/or  sell
copies  of  the Software,  and  to  permit persons  to  whom  the Software  is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE  IS PROVIDED "AS  IS", WITHOUT WARRANTY  OF ANY KIND,  EXPRESS OR
IMPLIED,  INCLUDING BUT  NOT  LIMITED TO  THE  WARRANTIES OF  MERCHANTABILITY,
FITNESS FOR  A PARTICULAR PURPOSE AND  NONINFRINGEMENT. IN NO EVENT  SHALL THE
AUTHORS  OR COPYRIGHT  HOLDERS  BE  LIABLE FOR  ANY  CLAIM,  DAMAGES OR  OTHER
LIABILITY, WHETHER IN AN ACTION OF  CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE  OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#pragma once

#include <memory>
#include <cassert>
#include <sstream>
#include <functional>

#include <llvm/IR/Value.h>

/// @brief Safe casting
template<typename From, typename To>
inline To* try_cast(From* from) {
    assert(from != nullptr);
    return dynamic_cast<To*>(from);
}

template<typename To, typename From>
inline To* cast_to(From from) {
    assert(from != nullptr);
    return dynamic_cast<To*>(from);
}

/// @brief Wrapper for try_cast to specifically take input from a unique_ptr*
template<typename From, typename To>
inline To* cast_from_uptr(std::unique_ptr<From>* from) {
    From* underlying = from->get();

    return try_cast<From, To>(underlying);
}

/// @brief span metadata for tokens
struct Span {
    std::string file;
    size_t line;
    size_t col;
    size_t start;
    size_t end;
};

struct Options {
    std::string input;
    std::string output;

    bool verbose : 1 = false;
    bool emit_object : 1 = false;
    bool emit_llvm : 1 = false;

    bool disable_color : 1 = false;

    bool dump_tokens : 1 = false;
    bool dump_ast : 1 = false;
    bool dump_analyzer : 1 = false;

    unsigned char opt_level = -1;
};

// If successfully casts, do x with it. Otherwise return NULL
template<typename To, typename Ret, typename From, class Func, typename ...Args>
inline Ret if_cast_then_do(From from, Func lambda, Args... args) {
    To* to = dynamic_cast<To*>(from);
    if(to) return lambda(to, args...);
    if constexpr (!std::is_void_v<Ret>) 
        return Ret{};
}

inline std::vector<std::string> split_string(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;
    while (std::getline(ss, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

/// @brief List of mangling schemes available
enum class ManglingScheme {
    None,
    Gearlang,
    Itanium,
    MSVC
};
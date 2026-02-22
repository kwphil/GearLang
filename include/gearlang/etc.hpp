#pragma once

#include <memory>

/// @brief A bit nicer syntax for a dynamic_cast
template<typename From, typename To>
inline To* try_cast(From* from) {
    return dynamic_cast<To*>(from);
}

/// @brief Wrapper for try_cast to specifically take input from a unique_ptr*
template<typename From, typename To>
inline To* cast_from_uptr(std::unique_ptr<From>* from) {
    From* underlying = from->get();

    return try_cast<From, To>(underlying);
}
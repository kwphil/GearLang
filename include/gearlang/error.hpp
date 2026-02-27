#pragma once

#include <string>
#include <fstream>
#include <vector>

namespace Error {
    enum class ErrorCodes {
        EXPECT_VALUE,
        UNEXPECTED_TOKEN,
        UNEXPECTED_EOF,
        UNKNOWN_TYPE,
        VARIABLE_ALREADY_DEFINED,
        VARIABLE_NOT_DEFINED,
        FUNCTION_NOT_DEFINED,
        INVALID_AST,
        BAD_TYPE,
    };

    /// @brief throws an error at a line. noreturn just to suppress warning about no func return.
    /// @param line_number the line to throw an error
    /// @param search_for will highlight a specific section of text. If it fails, it will search `leniency` lines down for it. If it still doesn't work, it will just not highlight anything. Pass "" to disable the feature
    /// @param err a specific message to throw
    /// @param leniency the number of lines to search through in case search_for fails
    /// @param code the error code to throw. .
    [[noreturn]] void throw_error (
        int line_number, 
        const char* search_for, 
        const char* err,
        ErrorCodes code,
        unsigned int leniency = 5
    );

    /// @brief Sets up the error management system
    /// @param filename the name of the file to open
    void setup_error_manager (const char* filename);
}
#include <string>

#include <fstream>
#include <iostream>
#include <sstream>

#include <stdlib.h>

#include "error.h"

std::string read_file(std::string);

int main(int argc, char** argv) {
    // First checking if the argument was provided correctly
    error_cond(argc != 2, ERROR_INV_ARGS);
    
    // Then we can read the file
    std::string input = read_file(argv[1]);

    error_cond(input.length() == 0, ERROR_INV_FILE);

    std::cout << input << std::endl;
    return EXIT_SUCCESS;
}

/// Just a helper function to clean up code for main
std::string read_file(std::string input) {
    std::stringstream buf;
    std::ifstream file(input);

    // Early exit if the file doesn't exist
    error_cond(file.fail(), ERROR_INV_FILE);

    buf << file.rdbuf();
    return buf.str();
}


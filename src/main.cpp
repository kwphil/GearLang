#include <string>

// Da streams
#include <fstream>
#include <iostream>
#include <sstream>

std::string read_file(std::string);

int main(int argc, char** argv) {
    // First checking if the argument was provided correctly
    if(argc != 2) {
        std::cerr << "Argument not provided correctly.";
        return 1;
    }
    
    // Then we can read the file
    std::string input = read_file(argv[1]);
    std::cout << input << std::endl;

    return EXIT_SUCCESS;
}

std::string read_file(std::string input) {
    std::stringstream buf;
    std::ifstream file(input);
    buf << file.rdbuf();
    return buf.str();
}


#include <string>
#include <fstream>
#include <iostream>
#include <vector>

#include <gearlang/error.hpp>

std::ifstream input_file;
std::vector<std::string> error_split_file;

void throw_error_no_highlight(
    int line_number,
    const char* err,
    Error::ErrorCodes code
) {
    // Print the error
    std::cerr << "Error: " << err << "\n";
    // Print highlighted line
    std::cerr << error_split_file[line_number];

    // Flush cerr
    std::cerr << std::endl;

    exit((int)code);
}

void Error::throw_error (
    int line_number, // This one is a copy so I'll be modifying it a little
    const char* _search_for, 
    const char* err,
    ErrorCodes code,
    int leniency
) {
    std::string search_for = _search_for;
    // if nothing to highlight
    if(search_for == "") {
        throw_error_no_highlight(line_number, err, code);
    }

    std::string current_line;
    int loc;
    
    // Looking for search_for
    for(int i = 0; i < leniency+1; i++) {
        current_line = error_split_file[line_number-1];
        loc = current_line.find(search_for);
        if(loc != std::string::npos) break;
        line_number++;
    }

    // If the program still can't find the contents, 
    // just call without highlights
    if(loc == std::string::npos) {
        std::cerr << "COMPILER WARNING: compiler could not find a match "
        "to string: " << search_for << "\n";
        throw_error_no_highlight(
            line_number,
            err,
            code
        );
    }

    // Print error
    std::cerr << "Error: " << err << "\n";

    // Print line
    std::cerr << current_line << "\n";
    
    // Offset highlight
    for(int i = 0; i < loc; i++) {
        std::cerr << " ";
    }

    // Print highlight
    for(int i = 0; i < search_for.length(); i++) {
        std::cerr << "^";
    }

    // flush cerr
    std::cerr << std::endl;

    exit((int)code);
}

void Error::setup_error_manager (const char* filename) {
    input_file.open(filename);

    std::string line;
    while(std::getline(input_file, line)) {
        error_split_file.push_back(line);
    }
}
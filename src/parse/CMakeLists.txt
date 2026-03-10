set(GEARLANG_PARSE
    base.cpp
    expr.cpp
    func.cpp
    stmt.cpp
    var.cpp
    literal.cpp
    expr_etc.cpp
    func_head.cpp
)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -fPIC -fsanitize=address")

add_library(gear_parse STATIC ${GEARLANG_PARSE}) 
target_include_directories(gear_parse PRIVATE 
    ${LLVM_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/../include
)

set_target_properties(gear_parse PROPERTIES
    OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/CMakeFiles/gear_parse.dir"
) 

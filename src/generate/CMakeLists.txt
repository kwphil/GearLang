set(GEARLANG_GENERATE
    base.cpp
    expr.cpp
    literal.cpp
    func.cpp
    branch.cpp
    stmt.cpp
    var.cpp
)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -fPIC -fsanitize=address")

add_library(gear_gen STATIC ${GEARLANG_GENERATE}) 
target_include_directories(gear_gen PRIVATE 
    ${LLVM_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/../include
)

set_target_properties(gear_gen PROPERTIES
    OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/CMakeFiles/gear_gen.dir"
)

cmake_minimum_required(VERSION 3.16)


include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)
# Locate GMP / GMPXX (used for arbitrary-precision solution counting)
find_path(GMP_INCLUDE_DIR NAMES gmp.h)
find_library(GMP_LIBRARY NAMES gmp)
find_library(GMPXX_LIBRARY NAMES gmpxx)
if(NOT GMP_INCLUDE_DIR OR NOT GMP_LIBRARY OR NOT GMPXX_LIBRARY)
    message(FATAL_ERROR "GMP/GMPXX not found. Install libgmp-dev (and libgmpxx).")
endif()

add_executable(d3x
main.cc
dancing_on_zdd.cc
dp_manager.cc
hidden_node_stack.cc
timer.cc)


target_include_directories(d3x PRIVATE ${GMP_INCLUDE_DIR})
target_link_libraries(d3x PRIVATE ${GMPXX_LIBRARY} ${GMP_LIBRARY})

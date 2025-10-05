cmake_minimum_required(VERSION 3.16)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

add_executable(d3x
main.cc 
dancing_on_zdd.cc
dp_manager.cc
hidden_node_stack.cc
batch_processor.cc
timer.cc)


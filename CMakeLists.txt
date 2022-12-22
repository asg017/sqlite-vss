cmake_minimum_required(VERSION 3.17 FATAL_ERROR)

project(myProject VERSION 0.1.0)


set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(/Users/alex/projects/sqlite-lines/sqlite)

#include_directories(/Users/alex/projects/faiss)
##include_directories(/Users/alex/projects/faissx/faiss/)

#link_directories(/Users/alex/projects/faiss/build/faiss)
##link_directories(/Users/alex/projects/faissx/faiss/build/faiss)

option(FAISS_ENABLE_GPU "" OFF)
option(FAISS_ENABLE_PYTHON "" OFF)
add_subdirectory(./faiss)

add_library(myProject SHARED src/main.cpp)
target_link_libraries(myProject faiss_avx2)
target_link_libraries(myProject sqlite3)
##target_link_libraries(myProject PRIVATE faiss)

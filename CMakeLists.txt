cmake_minimum_required(VERSION 3.17 FATAL_ERROR)

project(sqlite-vss VERSION 0.0.0)


set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#include_directories(/Users/alex/projects/faiss)
##include_directories(/Users/alex/projects/faissx/faiss/)

#link_directories(/Users/alex/projects/faiss/build/faiss)
##link_directories(/Users/alex/projects/faissx/faiss/build/faiss)

option(FAISS_ENABLE_GPU "" OFF)
option(FAISS_ENABLE_PYTHON "" OFF)
add_subdirectory(./faiss)
option(BUILD_TESTING "" OFF)

# Adding nlohmann_json for json parsing
set(JSON_BuildTests OFF CACHE INTERNAL "")
add_subdirectory(vendor/json)

# vendor in SQLite amalgammation
include_directories(vendor/sqlite-snapshot-202212231449)
link_directories(BEFORE vendor/sqlite-snapshot-202212231449)

add_library(sqlite-vss SHARED src/main.cpp)
target_link_libraries(sqlite-vss sqlite3)
target_link_libraries(sqlite-vss faiss_avx2)
target_link_libraries(sqlite-vss nlohmann_json::nlohmann_json)


##target_link_libraries(myProject PRIVATE faiss)

set_target_properties(sqlite-vss PROPERTIES PREFIX "")
set_target_properties(sqlite-vss PROPERTIES PREFIX "")
set_target_properties(sqlite-vss PROPERTIES OUTPUT_NAME "vss0")

add_executable(xxx src/debug.cpp)
target_link_libraries(xxx faiss_avx2)

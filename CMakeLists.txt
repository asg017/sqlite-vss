cmake_minimum_required(VERSION 3.17 FATAL_ERROR)
project(sqlite-vss VERSION 0.0.1.2)

configure_file(src/sqlite-vss.h.in sqlite-vss.h)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(FAISS_ENABLE_GPU "" OFF)
option(FAISS_ENABLE_PYTHON "" OFF)
option(BUILD_TESTING "" OFF)
add_subdirectory(./vendor/faiss)

  ## Adding nlohmann_json for json parsing
#set(JSON_BuildTests OFF CACHE INTERNAL "")
#add_subdirectory(vendor/json)

# vendor in SQLite amalgammation
include_directories(vendor/sqlite)
link_directories(BEFORE vendor/sqlite)

add_library(sqlite-vss SHARED src/extension.cpp)
target_link_libraries(sqlite-vss sqlite3)
target_link_libraries(sqlite-vss faiss_avx2)
#target_link_libraries(sqlite-vss nlohmann_json::nlohmann_json)
#target_include_directories(sqlite-vss PRIVATE vendor/sqlite-vector/include/)
target_include_directories(sqlite-vss PRIVATE /root/projects/sqlite-vector/include)
target_include_directories(sqlite-vss PUBLIC "${PROJECT_BINARY_DIR}")

set_target_properties(sqlite-vss PROPERTIES PREFIX "")
set_target_properties(sqlite-vss PROPERTIES PREFIX "")
set_target_properties(sqlite-vss PROPERTIES OUTPUT_NAME "vss0")

add_executable(xxx src/debug.cpp)
target_link_libraries(xxx faiss_avx2)

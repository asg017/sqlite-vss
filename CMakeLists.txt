cmake_minimum_required(VERSION 3.17 FATAL_ERROR)
project(sqlite-vss VERSION $ENV{SQLITE_VSS_CMAKE_VERSION})
project(sqlite-vector VERSION $ENV{SQLITE_VSS_CMAKE_VERSION})

if(PROJECT_VERSION_TWEAK)
  set(SQLITE_VSS_VERSION "v${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}-alpha.${PROJECT_VERSION_TWEAK}")
else()
  set(SQLITE_VSS_VERSION "v${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")
endif()
configure_file(src/sqlite-vss.h.in sqlite-vss.h)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(FAISS_ENABLE_GPU "" OFF)
option(FAISS_ENABLE_PYTHON "" OFF)
option(BUILD_TESTING "" OFF)
add_subdirectory(./vendor/faiss)

# vendor in SQLite amalgammation
include_directories(vendor/sqlite)
link_directories(BEFORE vendor/sqlite)

add_library(sqlite-vss SHARED src/vss-extension.cpp)
target_link_libraries(sqlite-vss sqlite3)
target_link_libraries(sqlite-vss faiss_avx2)
target_include_directories(sqlite-vss PUBLIC "${PROJECT_BINARY_DIR}")

set_target_properties(sqlite-vss PROPERTIES PREFIX "")
set_target_properties(sqlite-vss PROPERTIES PREFIX "")
set_target_properties(sqlite-vss PROPERTIES OUTPUT_NAME "vss0")
target_include_directories(sqlite-vss PRIVATE include/)

# Adding nlohmann_json for json parsing
set(JSON_BuildTests OFF CACHE INTERNAL "")
add_subdirectory(vendor/json)


add_library(sqlite-vector SHARED src/vector-extension.cpp)
target_link_libraries(sqlite-vector sqlite3)
target_link_libraries(sqlite-vector nlohmann_json::nlohmann_json)
target_include_directories(sqlite-vector PUBLIC "${PROJECT_BINARY_DIR}")

set_target_properties(sqlite-vector PROPERTIES PREFIX "")
set_target_properties(sqlite-vector PROPERTIES PREFIX "")
set_target_properties(sqlite-vector PROPERTIES OUTPUT_NAME "vector0")
target_include_directories(sqlite-vector PRIVATE include/)

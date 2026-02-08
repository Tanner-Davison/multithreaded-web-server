cmake_minimum_required(VERSION 3.15)
project(multithreaded-web-server
        VERSION 0.1
        DESCRIPTION "MultiThreaded Web Server"
        LANGUAGES CXX)

# Use ccache if available (works on both macOS and WSL)
find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
    message(STATUS "Using ccache")
endif()

# Use mold linker on Linux, default on macOS
if(NOT APPLE)
    find_program(MOLD_LINKER mold)
    if(MOLD_LINKER)
        add_link_options(-fuse-ld=mold)
        message(STATUS "Using mold linker")
    endif()
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Debug builds - fast compilation
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options(-Wall -Wextra -g -O0)
else()
    add_compile_options(-Wall -Wextra -O2)
endif()

# Your source files
set(SOURCES
    src/main.cpp
    src/Server.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Add pthread support for multithreading
find_package(Threads REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE Threads::Threads)

# Doxygen documentation (simplified)
find_package(Doxygen)
if(DOXYGEN_FOUND)
    add_custom_target(docs
        COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Generating API documentation with Doxygen"
        VERBATIM
    )
    message(STATUS "Doxygen found - use 'cmake --build build --target docs' to generate documentation")
else()
    message(STATUS "Doxygen not found")
endif()

message(STATUS "Using C++ compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

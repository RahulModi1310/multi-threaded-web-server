# Set the minimum version for CMake
cmake_minimum_required(VERSION 3.10)

# Define the project
project(multithreaded-server)

# Specify the C++ standard
set(CMAKE_CXX_STANDARD 20)

# Add include directories (for header files)
include_directories(${CMAKE_SOURCE_DIR}/include)

# Add the source files
file(GLOB SOURCES "src/*.cpp")

# Define the executable
add_executable(multithreaded-server ${SOURCES})

# Copy assets to the build directory (optional)
file(COPY ${CMAKE_SOURCE_DIR}/assets/ DESTINATION ${CMAKE_BINARY_DIR}/assets)

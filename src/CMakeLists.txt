cmake_minimum_required(VERSION 3.10)
project(OpenCVAulas)

# Make sure CMake can find OpenCV
find_package(OpenCV REQUIRED)

# Add your executable
add_executable(OpenCVAulas main.cpp)

# Link OpenCV libraries
target_link_libraries(OpenCVAulas PRIVATE ${OpenCV_LIBS})

# Add include directories
target_include_directories(OpenCVAulas PRIVATE ${OpenCV_INCLUDE_DIRS})
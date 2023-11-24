cmake_minimum_required(VERSION 3.24)
project(ocl_test)

set(CMAKE_CXX_STANDARD 17)

add_executable(ocl_test main.cpp computation/gpu/opencl_utils.cpp preprocessing/Preprocessor.cpp preprocessing/Preprocessor.h utils.h computation/CalculationScheduler.cpp computation/CalculationScheduler.h)

target_include_directories(ocl_test PRIVATE "${PROJECT_SOURCE_DIR}/OpenCL")
target_link_libraries(ocl_test "${PROJECT_SOURCE_DIR}/OpenCL/OpenCL.lib")

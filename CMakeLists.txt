cmake_minimum_required(VERSION 3.24)
project(ocl_test)

set(CMAKE_CXX_STANDARD 17)


add_executable(ocl_test main.cpp preprocessing/Preprocessor.cpp preprocessing/Preprocessor.h utils.h computation/CalculationScheduler.cpp computation/CalculationScheduler.h preprocessing/ParallelPreprocessor.cpp preprocessing/ParallelPreprocessor.cpp preprocessing/ParallelPreprocessor.h computation/ParallelCalculationScheduler.cpp computation/ParallelCalculationScheduler.h computation/gpu/OpenCLComponent.cpp computation/gpu/OpenCLComponent.h)
#target_compile_options(ocl_test PRIVATE /Ob1 /O2 /arch:AVX2 /Qpar)

target_include_directories(ocl_test PRIVATE "${PROJECT_SOURCE_DIR}/OpenCL")
target_link_libraries(ocl_test "${PROJECT_SOURCE_DIR}/OpenCL/OpenCL.lib")

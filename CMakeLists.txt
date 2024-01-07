cmake_minimum_required(VERSION 3.20)
project(ppr_ott)

set(CMAKE_CXX_STANDARD 17)


add_executable(ppr_ott
        main.cpp
        utils.h
        preprocessing/Preprocessor.cpp
        preprocessing/Preprocessor.h
        preprocessing/ParallelPreprocessor.cpp
        preprocessing/ParallelPreprocessor.cpp
        preprocessing/ParallelPreprocessor.h
        computation/CalculationScheduler.cpp
        computation/CalculationScheduler.h
        computation/ParallelCalculationScheduler.cpp
        computation/ParallelCalculationScheduler.h
        computation/gpu/OpenCLComponent.cpp
        computation/gpu/OpenCLComponent.h)
target_compile_options(ppr_ott PRIVATE /Qvec /Qvec-report:2 /arch:AVX2 /Qpar)

target_include_directories(ppr_ott PRIVATE "${PROJECT_SOURCE_DIR}/OpenCL")
target_link_libraries(ppr_ott "${PROJECT_SOURCE_DIR}/OpenCL/OpenCL.lib")

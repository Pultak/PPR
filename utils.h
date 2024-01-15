//
// Created by pulta on 28.10.2023.
//

#ifndef OCL_TEST_UTILS_H
#define OCL_TEST_UTILS_H

#include <sstream>
#include <utility>
#include <iostream>

#define TEXT_SEPARATOR "--------------------------------------"

#define VECTOR_SIZE_MACRO 16

#define DEFAULT_MAX_STEP_COUNT 10

#define DEFAULT_POPULATION_SIZE 100

#define DEFAULT_DESIRED_CORRELATION 0.9

#define DEFAULT_CONST_SCOPE 10.0

#define DEFAULT_POW_SCOPE 5

#define DEFAULT_SEED time(nullptr)

#define DEFAULT_GPU_NAME "DEFAULT"

#define DEFAULT_STEP_INFO_INTERVAL 5
const size_t VECTOR_SIZE = VECTOR_SIZE_MACRO;

//const char* time_format = "%Y-%m-%d %H:%M:%S";

template <class Function>
__int64 time_call(Function&& f)
{
    auto start_time = std::chrono::high_resolution_clock::now();
    f();
    // Record the end time
    auto end_time = std::chrono::high_resolution_clock::now();

    // Calculate the duration
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();

    return duration;
}

inline time_t parse_date(const std::string& date_string){
    std::tm tm{};
    std::stringstream ss(date_string);
    ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
    time_t time = mktime(&tm);
    return time;
}


struct input_parameters{
    const size_t max_step_count = DEFAULT_MAX_STEP_COUNT;
    const size_t population_size = DEFAULT_POPULATION_SIZE;
    const size_t seed = DEFAULT_SEED;

    const double desired_correlation = DEFAULT_DESIRED_CORRELATION;
    const double const_scope = DEFAULT_CONST_SCOPE;
    const int pow_scope = DEFAULT_POW_SCOPE;

    const std::string desired_gpu_name = DEFAULT_GPU_NAME;

    const bool parallel = false;

    const std::string input_folder;
    
    const size_t step_info_interval = DEFAULT_STEP_INFO_INTERVAL;
    
    explicit input_parameters(size_t max_step_count, size_t population_size, size_t seed, double desired_correlation,
                              double const_scope, int pow_scope, std::string desired_gpu_name, bool parallel,
                              std::string input_folder, size_t step_info_interval)
                              : max_step_count(max_step_count), population_size(population_size), seed(seed),
                              desired_correlation(desired_correlation), const_scope(const_scope), pow_scope(pow_scope),
                              desired_gpu_name(std::move(desired_gpu_name)), parallel(parallel),
                              input_folder(std::move(input_folder)), step_info_interval(step_info_interval){}

    explicit input_parameters() = default;

    void print_input_parameters(){
        std::cout << TEXT_SEPARATOR << std::endl;
        std::cout << "Running with these parameters:" << std::endl;
        std::cout << "Max_step_count: " << max_step_count << std::endl;
        std::cout << "Population_size: " << population_size << std::endl;
        std::cout << "Seed: " << seed << std::endl;
        std::cout << "Desired_correlation: " << desired_correlation << std::endl;
        std::cout << "Const_scope: " << const_scope << std::endl;
        std::cout << "Pow_scope: " << pow_scope << std::endl;
        std::cout << "Gpu_name: " << desired_gpu_name << std::endl;
        std::cout << "Parallel: " << parallel << std::endl;
        std::cout << "Input_folder: " << input_folder << std::endl;
        std::cout << "Step_info_interval: " << step_info_interval << std::endl;
        std::cout << TEXT_SEPARATOR << std::endl;
    }
};


#endif //OCL_TEST_UTILS_H

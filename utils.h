//
// Created by pulta on 28.10.2023.
//

#ifndef OCL_TEST_UTILS_H
#define OCL_TEST_UTILS_H

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

const size_t VECTOR_SIZE = 16;


struct input_parameters{
    const size_t max_step_count = 50;
    const size_t population_size = 100; //todo population size is raped to fit vectorization needs
    const size_t seed = 69; //todo actual time as seed to be random

    const double desired_correlation = 0.9; //todo do check to have it from 0-1

    const double const_scope = 1000.0;
    const int pow_scope = 5;
};


#endif //OCL_TEST_UTILS_H

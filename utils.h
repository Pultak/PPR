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





#endif //OCL_TEST_UTILS_H

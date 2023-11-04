//
// Created by pulta on 28.10.2023.
//

#ifndef OCL_TEST_CALCULATIONSCHEDULER_H
#define OCL_TEST_CALCULATIONSCHEDULER_H


#include <memory>
#include "../preprocessing/Preprocessor.h"
#include <array>


const size_t GENOME_CONSTANTS_SIZE = 4;
const size_t GENOME_POW_SIZE = 4;

struct genome{
    std::array<double, GENOME_CONSTANTS_SIZE> constants {};
    std::array<int, GENOME_POW_SIZE> powers {};
};

class CalculationScheduler {

public:
    //todo input parameters from args
    std::unique_ptr<genome> find_transformation_function(std::unique_ptr<input_vector>& input);


    std::vector<genome> init_population();

    double check_correlation(std::vector<genome> population, size_t &best_index);

    std::vector<genome> repopulate(const std::vector<genome>& old_population);
};


#endif //OCL_TEST_CALCULATIONSCHEDULER_H

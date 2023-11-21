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

    CalculationScheduler(const std::shared_ptr<input_data>& input);
    ~CalculationScheduler();


    //todo input parameters from args
    std::unique_ptr<genome> find_transformation_function();


    std::vector<genome> init_population();

    double check_correlation(const std::vector<genome>& population, size_t &best_index);

    std::vector<genome> repopulate(const std::vector<genome>& old_population, genome& best_genome);

private:

    const std::shared_ptr<input_data> input;
    double* transformation_result;
    double* corr_result;

    double hr_sum;
    double squared_hr_corr_sum;

    std::mt19937 corr_value_generator{69}; // todo seed from parameter
    std::uniform_real_distribution<> corr_uniform_real_distribution{0, 1};

private:

    void transform(const genome& genome1);
    void calculate_hr_init_data();

    const genome* get_parent(const std::vector<genome> &vector, size_t &last_index);
};


#endif //OCL_TEST_CALCULATIONSCHEDULER_H

//
// Created by pulta on 28.10.2023.
//

#ifndef OCL_TEST_CALCULATIONSCHEDULER_H
#define OCL_TEST_CALCULATIONSCHEDULER_H


#include <memory>
#include "../preprocessing/Preprocessor.h"
#include "../utils.h"
#include <array>


const size_t GENOME_CONSTANTS_SIZE = 4;
const size_t GENOME_POW_SIZE = 4;

struct genome{
    std::array<double, GENOME_CONSTANTS_SIZE> constants {};
    std::array<uint8_t, GENOME_POW_SIZE> powers {};
};

class CalculationScheduler {

public:

    CalculationScheduler(const std::shared_ptr<input_data>& input, const input_parameters& input_params);
    ~CalculationScheduler();

    //todo input parameters from args
    double find_transformation_function(genome& best_genome);

    void init_population(std::vector<genome>& init_population) const;

    double check_correlation(const std::vector<genome>& population, size_t &best_index);

    void repopulate(const std::vector<genome>& old_population, std::vector<genome>& new_population,
                                   genome& best_genome);

private:

    const std::shared_ptr<input_data> input;
    std::vector<double> transformation_result;
    std::vector<double> corr_result;

    double hr_sum = 0;
    double squared_hr_corr_sum = 0;

    std::mt19937 corr_value_generator;
    std::uniform_real_distribution<> corr_uniform_real_distribution{0, 1};

    const input_parameters input_params;

private:

    void transform(const genome& genome1);
    void calculate_hr_init_data();

    const genome* get_parent(const std::vector<genome> &vector, size_t &last_index);
};


#endif //OCL_TEST_CALCULATIONSCHEDULER_H

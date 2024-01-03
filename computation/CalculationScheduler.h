//
// Created by pulta on 28.10.2023.
//

#ifndef OCL_TEST_CALCULATIONSCHEDULER_H
#define OCL_TEST_CALCULATIONSCHEDULER_H


#include <memory>
#include "../preprocessing/Preprocessor.h"
#include "../utils.h"
#include <array>
#include <iostream>


const size_t GENOME_CONSTANTS_SIZE = 4;
const size_t GENOME_POW_SIZE = 4;

struct genome{
    std::array<double, GENOME_CONSTANTS_SIZE> constants {};
    std::array<unsigned char, GENOME_POW_SIZE> powers {};
};

inline void print_genome(const genome &best_genome) {
    auto& c = best_genome.constants;
    auto& p = best_genome.powers;
    std::cout << "Best polynomial: "    << c[0] << "x^" << (int)p[0] << " + "
              << c[1] << "y^" << (int)p[1] << " + "
              << c[2] << "z^" << (int)p[2] << " + "
              << c[3] << std::endl;
}

class CalculationScheduler {

public:
    bool running = true;
    std::vector<double> transformation_result;

public:

    CalculationScheduler(const std::shared_ptr<input_data>& input, const input_parameters& input_params);
    ~CalculationScheduler();

    double find_transformation_function(genome& best_genome);

    void init_population(std::vector<genome>& init_population) const;

    virtual double check_correlation(const std::vector<genome>& population, size_t &best_index);

    void repopulate(const std::vector<genome>& old_population, std::vector<genome>& new_population,
                                   genome& best_genome);

    void transform(const genome& genome1);

protected:

    const std::shared_ptr<input_data> input;
    std::vector<double> corr_result;

    const double hr_sum = 0;
    const double squared_hr_corr_sum = 0;

    std::mt19937 corr_value_generator;
    std::uniform_real_distribution<> corr_uniform_real_distribution{0, 0.1};

    const input_parameters input_params;

protected:

    virtual void init_calculation();

    const genome* get_parent(const std::vector<genome> &vector, size_t &last_index);

    [[nodiscard]] double get_abs_correlation_value(double entries_count, double acc_sum,
                                     double acc_sum_pow_2, double hr_acc_sum) const;
};


#endif //OCL_TEST_CALCULATIONSCHEDULER_H

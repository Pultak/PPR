//
// Created by pulta on 28.10.2023.
//

#include <random>
#include <iostream>
#include "CalculationScheduler.h"


CalculationScheduler::CalculationScheduler(const std::shared_ptr<input_data>& input,
                                           const input_parameters& input_params): input(input), input_params(input_params),
                                                                                  corr_value_generator(input_params.seed),
                                                                                  transformation_result(input->acc_entries_count),
                                                                                  corr_result(input_params.population_size),
                                                                                  hr_sum(input->hr_sum),
                                                                                  squared_hr_corr_sum(input->squared_hr_corr_sum){

}

CalculationScheduler::~CalculationScheduler() = default;


double CalculationScheduler::find_transformation_function(genome& best_genome) {
    init_calculation();

    std::vector<genome> curr_population (this->input_params.population_size);
    std::vector<genome> old_population (this->input_params.population_size);

    __int64 elapsed = time_call([&] {
        init_population(curr_population);
    });
    std::cout << "Initialization of first population took " << elapsed << " ms" << std::endl;

    uint32_t step_done_count = 0;
    double max_corr = 0;
    size_t best_index = 0;
    const auto desired_corr = this->input_params.desired_correlation;

    while(step_done_count < this->input_params.max_step_count && this->running){

        elapsed = time_call([&] {
            max_corr = check_correlation(curr_population, best_index);
        });
        std::cout << "Transform and correlation calculation took " << elapsed << " ms" << std::endl;

        if(max_corr > desired_corr){
            std::cout << "Maximal (desired) correlation threshold reached (" << max_corr << ">" << desired_corr
                        << ")! Stopping at " << step_done_count << "th step!" << std::endl;
            break;
        }
        if(step_done_count % this->input_params.step_info_interval == 0){
            std::cout << step_done_count + 1 << "th step was done. Current best correlation: " << max_corr << std::endl;
            print_genome(curr_population[best_index]);
        }

        auto swap = curr_population;
        curr_population = old_population; //swap pointers so we don't have to allocate another vector
        old_population = swap;

        elapsed = time_call([&] {
            repopulate(old_population, curr_population, old_population[best_index]);
        });
        std::cout << "Generation of new population took " << elapsed << " ms" << std::endl;

        //after new repopulation the best genome is at the first position again
        best_index = 0;
        ++step_done_count;
    }
    std::memcpy(&best_genome, &curr_population[best_index], sizeof(genome));
    return max_corr;
}

void CalculationScheduler::init_population(std::vector<genome>& init_population) const {
    srand(this->input_params.seed);
//    std::random_device rd; // obtain a random number from hardware
    std::mt19937 generator(this->input_params.seed); // seed the generator

    std::uniform_real_distribution<> uniformRealDistribution(-this->input_params.const_scope, this->input_params.const_scope); // define the range
    std::uniform_int_distribution<> uniformIntDistribution(1, this->input_params.pow_scope); // define the range

    for(size_t i = 0; i < this->input_params.population_size; i++){
        genome genome {};

        //init genome constants
        for(auto& cons : genome.constants){
            cons = uniformRealDistribution(generator);
        }
        for(auto& pow : genome.powers){
            pow = uniformIntDistribution(generator);
        }
        init_population[i] = genome;
    }
}

double CalculationScheduler::check_correlation(const std::vector<genome>& population, size_t &best_index) {

    double best_corr = 0;
    size_t gen_index = 0;
    const auto entries_count = (double)this->input->hr_entries_count;
    const auto& hr = this->input->hr->values;
    for (const genome gen: population) {
        transform(gen);

        double acc_sum = 0;
        double acc_sum_pow_2 = 0;
        double hr_acc_sum = 0;
        for (int i = 0; i < entries_count; ++i) {
            auto trs_acc_unit = this->transformation_result[i];
            acc_sum += trs_acc_unit;
            acc_sum_pow_2 += (trs_acc_unit * trs_acc_unit);
            hr_acc_sum += (trs_acc_unit * hr[i]);
        }

        double corr_abs = get_abs_correlation_value(entries_count, acc_sum, acc_sum_pow_2, hr_acc_sum);

        if(best_index != gen_index && corr_abs > best_corr){
            best_corr = corr_abs;
            best_index = gen_index;
        }
        this->corr_result[gen_index] = corr_abs;
        ++gen_index;
    }
    return best_corr;
}

double CalculationScheduler::get_abs_correlation_value(const double entries_count, double acc_sum, double acc_sum_pow_2,
                                                       double hr_acc_sum) const {
    auto divident = (entries_count * hr_acc_sum) - (hr_sum * acc_sum);

    auto divisor1 = sqrt(squared_hr_corr_sum);
    auto divisor2 = sqrt(entries_count * acc_sum_pow_2) - (acc_sum * acc_sum);

    if(std::isnan(divisor1) || std::isnan(divisor2) || std::isnan(divident)){
        //the number is too big
        return 0;
    }
    double corr = divident / (divisor1 * divisor2);

    double corr_abs = abs(corr);
    return corr_abs;
}

void CalculationScheduler::repopulate(const std::vector<genome>& old_population,
                                                     std::vector<genome>& new_population, genome& best_genome) {
    const size_t population_size = this->input_params.population_size;


    //copy the best genome to the next gen
    std::memcpy(&new_population[0], &best_genome, sizeof(genome));
    size_t current_index = 1;
    size_t last_parent_index = 0;
    while(current_index < population_size){
        auto parent1 = get_parent(old_population, last_parent_index);
        auto parent2 = get_parent(old_population, last_parent_index);

        std::memcpy(&new_population[current_index].powers,
                    &parent1->powers, GENOME_CONSTANTS_SIZE);
        std::memcpy(&new_population[current_index].constants,
                    &parent2->constants, GENOME_POW_SIZE * sizeof(double));
        ++current_index;
    }
}

void CalculationScheduler::transform(const genome& current_genome) {
    const auto& acc_x = input->acc_z;
    const auto& acc_y = input->acc_z;
    const auto& acc_z = input->acc_z;

    const auto& c = current_genome.constants;
    const auto& p = current_genome.powers;

    const auto vector_size = input->acc_entries_count;
    for(size_t i = 0; i < vector_size; ++i){
        double x = acc_x->values[i];
        double y = acc_y->values[i];
        double z = acc_z->values[i];

        this->transformation_result[i] = c[0] * pow(x, p[0]) + c[1] * pow(y, p[1])
                + c[2] * pow(z, p[2]) + c[3];
    }
}


const genome* CalculationScheduler::get_parent(const std::vector<genome> &vector, size_t &last_index) {

    const auto population_size = this->input_params.population_size;
    double corr_needed = this->corr_uniform_real_distribution(this->corr_value_generator);
    double curr_corr_sum = 0;
    while(curr_corr_sum < corr_needed){
        ++last_index;
        if(last_index >= population_size){
            last_index = 0;
        }
        curr_corr_sum += this->corr_result[last_index];
    }

    return &vector[last_index];
}

void CalculationScheduler::init_calculation() {

}

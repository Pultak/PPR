//
// Created by pulta on 28.10.2023.
//

#include <random>
#include "CalculationScheduler.h"



CalculationScheduler::CalculationScheduler(const std::shared_ptr<input_data>& input,
                                           const input_parameters& input_params): input(input), input_params(input_params),
                                                                                  corr_value_generator(input_params.seed),
                                                                                  transformation_result(input_params.population_size),
                                                                                  corr_result(input_params.population_size){
    calculate_hr_init_data();
}

CalculationScheduler::~CalculationScheduler() = default;

double CalculationScheduler::find_transformation_function(genome& best_genome) {

    std::vector<genome> old_population {this->input_params.population_size};
    std::vector<genome> new_population {this->input_params.population_size};
    init_population(new_population);

    uint32_t step_done_count = 0;
    double max_corr = 0;
    size_t best_index = 0;

    while(step_done_count < this->input_params.max_step_count){
        max_corr = check_correlation(new_population, best_index);
        ++step_done_count;
        if(max_corr > this->input_params.desired_correlation){
            break;
        }
        auto swap = new_population; //todo is swap ok ?
        new_population = old_population; //swap pointers so we don't have to allocate another vector
        old_population = swap;

        repopulate(old_population, new_population, old_population[best_index]);
        //after new repopulation the best genome is at the first position again
        best_index = 0;
    }
    //todo destroy objects + close graphics card etc.
    std::memcpy(&best_genome, &new_population[best_index], sizeof(genome));
    return max_corr;
}

void CalculationScheduler::init_population(std::vector<genome>& init_population) const {
    srand(this->input_params.seed);
//    std::random_device rd; // obtain a random number from hardware
    std::mt19937 generator(this->input_params.seed); // seed the generator

    std::uniform_real_distribution<> uniformRealDistribution(-this->input_params.const_scope, this->input_params.const_scope); // define the range
    std::uniform_int_distribution<> uniformIntDistribution(1, this->input_params.pow_scope); // define the range

    for(int i = 0; i < this->input_params.population_size; i++){//todo parallel init
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
    for (const genome gen: population) {
        transform(gen);

        double acc_sum = 0;
        double acc_sum_pow_2 = 0;
        double hr_acc_sum = 0;
        for (int i = 0; i < this->input->entries_count; ++i) { //todo parallel shh
            auto trs_acc_unit = this->transformation_result[i];
            acc_sum += trs_acc_unit;
            acc_sum_pow_2 += trs_acc_unit * trs_acc_unit;
            hr_acc_sum += trs_acc_unit * this->input->hr->values[i]; //todo slow?
        }

        double corr = abs((this->input->entries_count * hr_acc_sum) - this->hr_sum * acc_sum)
                / (this->squared_hr_corr_sum * sqrt((this->input->entries_count * acc_sum_pow_2) - acc_sum * acc_sum));

        if(corr > best_corr){
            best_corr = corr;
            best_index = gen_index;
        }
        this->corr_result[gen_index] = corr;
        ++gen_index;
    }
    return best_corr;
}

void CalculationScheduler::repopulate(const std::vector<genome>& old_population,
                                                     std::vector<genome>& new_population, genome& best_genome) {
    size_t population_size = this->input->entries_count;

    size_t current_index = 0;

    //copy the best genome to the next gen

    std::memcpy(&new_population[0], &best_genome, sizeof(genome));
    ++current_index;
    size_t last_parent_index = 0;
    while(current_index < population_size){ //todo parallel shit
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
    auto& acc_x = input->acc_z;
    auto& acc_y = input->acc_z;
    auto& acc_z = input->acc_z;

    auto& c = current_genome.constants;
    auto& p = current_genome.powers;

    for(int i = 0; i < input->entries_count; ++i){ //todo parallel shh
        double x = acc_x->values[i];
        double y = acc_y->values[i];
        double z = acc_z->values[i];

        //todo better way?
        this->transformation_result[i] = c[0] * pow(x, p[0]) + c[1] * pow(y, p[1])
                + c[2] * pow(z, p[2]) + c[3];
    }
}

void CalculationScheduler::calculate_hr_init_data() {

    double sum = 0;
    double sum_power_2 = 0;

    for (const auto hr: this->input->hr->values) {
        sum += hr;
        sum_power_2 += hr * hr;
    }

    this->hr_sum = sum;
    this->squared_hr_corr_sum = sqrt(this->input->entries_count * sum_power_2 - (sum * sum));
}

const genome* CalculationScheduler::get_parent(const std::vector<genome> &vector, size_t &last_index) {

    double corr_needed = this->corr_uniform_real_distribution(this->corr_value_generator);
    double curr_corr_sum = 0;
    while(curr_corr_sum < corr_needed){
        curr_corr_sum += this->corr_result[last_index];
        ++last_index;
        if(last_index > this->input->entries_count){
            last_index = 0;
        }
    }

    return &vector[last_index];
}


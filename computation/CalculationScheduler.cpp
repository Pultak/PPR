//
// Created by pulta on 28.10.2023.
//

#include <random>
#include "CalculationScheduler.h"



CalculationScheduler::CalculationScheduler(const std::shared_ptr<input_data>& input): input(input){
    transformation_result = static_cast<double *>(calloc(sizeof(double), input->entries_count));
    calculate_hr_init_data();
}

CalculationScheduler::~CalculationScheduler() {
    free(transformation_result);
}

std::unique_ptr<genome> CalculationScheduler::find_transformation_function() {

    size_t max_step_count = 1000;

    //todo check if serial or parallel computation
    auto population = init_population();

    uint32_t step_done_count = 0;
    //todo passed by param
    double max_corr = 0;
    size_t best_index = 0;
    while(step_done_count < max_step_count){
        max_corr = check_correlation(population, best_index);
        ++step_done_count;
        if(max_corr > 0.9){
            break;
        }
        population = repopulate(population, population[best_index]);
        //after new repopulation the best genome is at the first position again
        best_index = 0;
    }
    //todo destroy objects + close graphics card
    std::unique_ptr best_corr = std::make_unique<genome>(population[best_index]); //todo function move to pass the data?

    return std::move(best_corr);
}

std::vector<genome> CalculationScheduler::init_population() {
    //todo args ml parameters
    size_t population_size = 1000; //todo genome size is raped to fit vectorization needs
    double const_scope = 1000.0;
    int pow_scope = 5;
    int seed = 69;

    srand(seed);
//    std::random_device rd; // obtain a random number from hardware
    std::mt19937 generator(seed); // seed the generator

    std::uniform_real_distribution<> uniformRealDistribution(-const_scope, const_scope); // define the range
    std::uniform_int_distribution<> uniformIntDistribution(0, pow_scope); // define the range

    std::vector<genome> init_population {population_size};
    for(int i = 0; i < population_size; i++){
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
    return init_population;
}

double CalculationScheduler::check_correlation(const std::vector<genome>& population, size_t &best_index) {

    double best_corr = 0;
    size_t gen_index = 0;
    for (const genome gen: population) {
        transform(gen);

        double acc_sum = 0;
        double acc_sum_pow_2 = 0;
        double hr_acc_sum = 0;
        for (int i = 0; i < this->input->entries_count; ++i) {
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
        ++gen_index;
    }
    return best_corr;
}

//todo this should be unique_ptr cause we loose values otherwise
std::vector<genome> CalculationScheduler::repopulate(const std::vector<genome>& old_population, genome& best_genome) {
    size_t population_size = this->input->entries_count;

    auto result = std::vector<genome>(population_size);
    size_t current_index = 0;

    //copy the best genome to the next gen
    //todo this is sus
    std::memcpy(&result[0], &best_genome, sizeof(genome));
    ++current_index;
    size_t last_parent_index = 0;
    while(current_index < population_size){
        auto parent1 = get_parent(old_population, last_parent_index);
        auto parent2 = get_parent(old_population, last_parent_index);

        std::memcpy(&result[current_index].powers, &parent1->powers, sizeof(genome)); //todo sus
        std::memcpy(&result[current_index].constants, &parent2->constants, sizeof(genome));
        ++current_index;
    }
    return result;
}

void CalculationScheduler::transform(const genome& current_genome) {
    auto& acc_x = input->acc_z;
    auto& acc_y = input->acc_z;
    auto& acc_z = input->acc_z;

    auto& c = current_genome.constants;
    auto& p = current_genome.powers;

    for(int i = 0; i < input->entries_count; ++i){
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

    double corr_needed = corr_uniform_real_distribution(corr_value_generator);
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


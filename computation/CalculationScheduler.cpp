//
// Created by pulta on 28.10.2023.
//

#include <random>
#include "CalculationScheduler.h"

std::unique_ptr<genome> CalculationScheduler::find_transformation_function(std::unique_ptr<input_vector> &input) {

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
            //todo destroy objects + close graphics card
            return std::make_unique<genome>(population[best_index]);
        }
        population = repopulate(population);
    }
    //todo destroy objects + close graphics card
    return std::make_unique<genome>(population[best_index]);
}

std::vector<genome> CalculationScheduler::init_population() {
    //todo args ml parameters
    size_t population_size = 1000; //todo genome size is raped to fit vectorization needs
    double const_scope = 1000.0;
    int pow_scope = 5;
    int seed = 69;

    srand(seed);
    std::random_device rd; // obtain a random number from hardware
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

double CalculationScheduler::check_correlation(std::vector<genome> population, size_t &best_index) {

}

std::vector<genome> CalculationScheduler::repopulate(const std::vector<genome>& old_population) {





    return std::vector<genome>();
}

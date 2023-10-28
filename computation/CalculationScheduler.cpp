//
// Created by pulta on 28.10.2023.
//

#include "CalculationScheduler.h"

void CalculationScheduler::find_transformation_function(std::unique_ptr<input_vector> &input) {

    //todo check if serial or parallel computation
    init_population();

    uint32_t step_done_count = 0;
    //todo passed by param
    double max_corr = 0;
    while(step_done_count < 1000){
        max_corr = step();
        ++step_done_count;
        if(max_corr > 0.9){

            return;
        }
    }
}

void CalculationScheduler::init_population() {
    //todo population size is rped to fit vectorization needs
}

double CalculationScheduler::step() {

}

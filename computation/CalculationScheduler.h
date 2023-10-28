//
// Created by pulta on 28.10.2023.
//

#ifndef OCL_TEST_CALCULATIONSCHEDULER_H
#define OCL_TEST_CALCULATIONSCHEDULER_H


#include <memory>
#include "../preprocessing/Preprocessor.h"


struct population{
    vector<>
};

class CalculationScheduler {

public:
    //todo input parameters from args
    void find_transformation_function(std::unique_ptr<input_vector>& input);


    void init_population();

    double step();
};


#endif //OCL_TEST_CALCULATIONSCHEDULER_H

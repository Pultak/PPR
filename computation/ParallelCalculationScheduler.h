//
// Created by pulta on 27.11.2023.
//

#ifndef OCL_TEST_PARALLELCALCULATIONSCHEDULER_H
#define OCL_TEST_PARALLELCALCULATIONSCHEDULER_H


#include "CalculationScheduler.h"
#include "gpu/OpenCLComponent.h"

class ParallelCalculationScheduler : public CalculationScheduler {

public:
    ParallelCalculationScheduler(OpenCLComponent& cl, const std::shared_ptr<input_data>& input,
                                 const input_parameters& input_params);

protected:

    void init_calculation() override;

    double check_correlation(const std::vector<genome>& population, size_t &best_index) override;

private:
    OpenCLComponent& cl_device;

    /// output vector of the reduce function from gpu device
    std::vector<std::array<std::vector<double>, 3>> sum_reduce_result;

};


#endif //OCL_TEST_PARALLELCALCULATIONSCHEDULER_H

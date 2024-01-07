//
// Created by pulta on 27.11.2023.
//

#include <execution>
#include "ParallelCalculationScheduler.h"


ParallelCalculationScheduler::ParallelCalculationScheduler(OpenCLComponent& cl,
                                                           const std::shared_ptr<input_data>& input,
                                                           const input_parameters& input_params)
                                                           : CalculationScheduler(input, input_params), cl_device(cl){

}

void ParallelCalculationScheduler::init_calculation() {
    CalculationScheduler::init_calculation();

    const size_t entries_count = this->input->hr_entries_count;
    const size_t work_groups_count = ceil((double)entries_count
                                          / (double)this->cl_device.work_group_size);

    this->sum_reduce_result = {this->input_params.population_size,
                              {std::vector<double>(work_groups_count),
                               std::vector<double>(work_groups_count),
                               std::vector<double>(work_groups_count)
                              }};
}


double ParallelCalculationScheduler::check_correlation(const std::vector<genome>& population, size_t &best_index) {

    const cl::size_type numbers_bytes_size = sizeof(double) * input->hr_entries_count;
    const size_t entries_count = this->input->hr_entries_count;
    const size_t work_groups_count = ceil((double)entries_count
                                                    / (double)this->cl_device.work_group_size);
    double best_corr = 0;
    size_t gen_index = 0;

    std::vector<cl::Event> synch_events(this->input_params.population_size);
#if defined(CL_HPP_ENABLE_EXCEPTIONS)
    try{
#endif
        //check if buffers are already initialized
        cl_device.init_static_buffers(input, numbers_bytes_size, work_groups_count);
        //first assign the jobs to gpu queue
        for(const auto& curr_gen: population) {
            //every genome has its on result buffers for sum reduce
            auto& result = this->sum_reduce_result[gen_index];
            cl_device.calculate_correlation(curr_gen, result,
                                            entries_count, work_groups_count, synch_events[gen_index]);
            ++gen_index;
        }

        synch_events[gen_index - 1].wait();
        gen_index = 0;

        //now for every sum reduce calculate correlation
        for(auto& [out_acc, out_acc2, out_acc_hr]: this->sum_reduce_result){
            double acc_sum = 0, acc_sum_pow_2 = 0, hr_acc_sum = 0;

            // sum the partial sums
            #pragma loop(hint_parallel(VECTOR_SIZE_MACRO))
            for(size_t i = 0; i < work_groups_count; ++i){
                acc_sum += out_acc[i];
                acc_sum_pow_2 += out_acc2[i];
                hr_acc_sum += out_acc_hr[i];
            }

            double corr_abs = this->get_abs_correlation_value(static_cast<double>(entries_count),
                                                              acc_sum, acc_sum_pow_2, hr_acc_sum);
            if(corr_abs > best_corr){
                best_corr = corr_abs;
                best_index = gen_index;
            }
            this->corr_result[gen_index] = corr_abs;
            if(gen_index < 15){
                std::cout << corr_abs << std::endl; //todo remove
            }
            ++gen_index;
        }

#if defined(CL_HPP_ENABLE_EXCEPTIONS)
    } catch (cl::Error err) {
        std::cerr << "Error occurred during gpu computation: " << err.what() << "(" << err.err() << " - "
                  << cl_device.Get_OpenCL_Error_Desc(err.err()) << ")" << std::endl;
         exit(1);
    }
#endif

    return best_corr;
}

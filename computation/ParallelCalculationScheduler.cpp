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



}



double ParallelCalculationScheduler::check_correlation(const std::vector<genome>& population, size_t &best_index) {



    const cl::size_type numbers_bytes_size = sizeof(double) * input->hr_entries_count;
    const auto work_groups_count = numbers_bytes_size / this->cl_device.work_group_size;

    std::vector<double> out_sum_acc(work_groups_count);
    std::vector<double> out_sum_acc2(work_groups_count);
    std::vector<double> out_sum_acc_hr(work_groups_count);

    double best_corr = 0;
    size_t gen_index = 0;

    const auto entries_count = (double)this->input->hr_entries_count;

#if defined(CL_HPP_ENABLE_EXCEPTIONS)
    try{
#endif
        cl_device.init_static_buffers(input, numbers_bytes_size);

        for(const auto& curr_gen: population) {
            cl_device.calculate_correlation(curr_gen, out_sum_acc, out_sum_acc2,
                                            out_sum_acc_hr, numbers_bytes_size);

            double acc_sum = 0, acc_sum_pow_2 = 0, hr_acc_sum = 0;

            std::for_each(/*std::execution::par,*/ begin(out_sum_acc), end(out_sum_acc),
                                                   [&](const double value) {
                acc_sum += value;
            });

            std::for_each(/*std::execution::par,*/ begin(out_sum_acc2), end(out_sum_acc2),
                                                   [&](const double value) {
               acc_sum_pow_2 += value;
            });

            std::for_each(/*std::execution::par,*/ begin(out_sum_acc_hr), end(out_sum_acc_hr),
                                                   [&](const double value) {
               hr_acc_sum += value;
            });

            double corr_abs = this->get_abs_correlation_value(entries_count, acc_sum, acc_sum_pow_2, hr_acc_sum);
            if(corr_abs > best_corr){
                best_corr = corr_abs;
                best_index = gen_index;
            }
            this->corr_result[gen_index] = corr_abs;
        }

#if defined(CL_HPP_ENABLE_EXCEPTIONS)
    } catch (cl::Error err) {
        std::cerr << "Error occurred during gpu computation: " << err.what() << "(" << err.err() << " - "
                  << cl_device.Get_OpenCL_Error_Desc(err.err()) << ")" << std::endl;
        //todo exit handlers for free buffers
        exit(0);
    }
#endif

    return best_corr;
}

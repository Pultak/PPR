//
// Created by pulta on 27.11.2023.
//

#ifndef OCL_TEST_OPENCLCOMPONENT_H
#define OCL_TEST_OPENCLCOMPONENT_H


#include "CL/opencl.hpp"
#include "../../preprocessing/Preprocessor.h"
#include "../CalculationScheduler.h"

class OpenCLComponent {

public:
    OpenCLComponent();
    ~OpenCLComponent();

    [[nodiscard]] bool was_init_successful() const;

public:
    void calculate_correlation(const genome &curr_gen, std::vector<double> &out_sum_acc,
                               std::vector<double> &out_sum_acc2, std::vector<double> &out_sum_acc_hr,
                               cl::size_type numbers_bytes_size);

    void init_static_buffers(const std::shared_ptr<input_data> &input, size_t numbers_bytes_size);

    const char* Get_OpenCL_Error_Desc(cl_int error) noexcept;

public:
    size_t work_group_size = 0;

private:
    cl::Device selected_device;
    cl_int build_err = CL_SUCCESS;

    size_t global_mem_size;
    size_t local_mem_size;

    std::unique_ptr<const cl::Context> device_context;

    std::unique_ptr<cl::CommandQueue> trans_queue;
    std::unique_ptr<cl::CommandQueue> corr_queue;

    std::unique_ptr<cl::Kernel> transform_kernel;
    std::unique_ptr<cl::Kernel> correlation_kernel;

    bool buffers_initialized = false;
    std::unique_ptr<const cl::Buffer> hr_vector = nullptr;
    std::unique_ptr<const cl::Buffer> x_acc_vector = nullptr;
    std::unique_ptr<const cl::Buffer> y_acc_vector = nullptr;
    std::unique_ptr<const cl::Buffer> z_acc_vector = nullptr;


    std::unique_ptr<const cl::Buffer> transform_result = nullptr;

    std::unique_ptr<cl::Buffer> transform_vector = nullptr;

private:

    cl::Device try_select_first_gpu();
    void dump_build_log(cl::Program& program);
    void transform_acc(cl::size_type numbers_bytes_size, const genome& current_gen,
                       cl::Event& trans_event);
    void snitch_device_info(cl::Device& device);

};


#endif //OCL_TEST_OPENCLCOMPONENT_H

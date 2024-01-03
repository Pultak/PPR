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
    OpenCLComponent(cl::Device selected_device, cl::Context device_context,
                    cl::Kernel full_correlation_kernel, size_t work_group_size);
    ~OpenCLComponent();

public:
    static void init_opencl_device(std::unique_ptr<OpenCLComponent> &cl_device, const std::string& desired_gpu_device);

    void calculate_correlation(const genome &curr_gen, std::array<std::vector<double>, 3>& out_sums,
                               const cl::size_type entries_count, const size_t work_groups_count,
                               cl::Event &corr_kernel_event);

    void init_static_buffers(const std::shared_ptr<input_data> &input, size_t numbers_bytes_size,
                             const size_t work_groups_count);

    static const char* Get_OpenCL_Error_Desc(cl_int error) noexcept;

public:
    const size_t work_group_size;

private:
    cl::Device selected_device;

    cl::Context device_context;

    cl::Kernel full_corr_kernel;

    bool buffers_initialized = false;
    cl::Buffer hr_vector;
    cl::Buffer x_acc_vector;
    cl::Buffer y_acc_vector;
    cl::Buffer z_acc_vector;

private:

    static cl::Device select_gpu(const std::string &desired_gpu_device);
    static cl_int dump_build_log(cl::Program& program);
};


#endif //OCL_TEST_OPENCLCOMPONENT_H

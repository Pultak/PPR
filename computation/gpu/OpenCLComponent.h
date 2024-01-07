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

    /// Function enqueuing kernel with passed genome and writing it into desire buffers
    /// \param curr_gen current function genome for accelator data transformation
    /// \param out_sums output buffers with partial sums
    /// \param entries_count count of data entries parsed from input files
    /// \param work_groups_count count of work group present for this GPU device
    /// \param corr_kernel_event event that can be used to synchronize with gpu processes
    void calculate_correlation(const genome &curr_gen, std::array<std::vector<double>, 3>& out_sums,
                               const cl::size_type entries_count, const size_t work_groups_count,
                               cl::Event &corr_kernel_event);

    /// function used to initialize buffers that dont change
    /// \param input input data processed from the input files
    /// \param numbers_bytes_size count of bytes needed for data buffers
    /// \param work_groups_count count of work group present for this GPU device
    void init_static_buffers(const std::shared_ptr<input_data> &input, size_t numbers_bytes_size,
                             const size_t work_groups_count);

    /// Transform openCL error number into more readable string
    /// \param error integer value of openCL error
    /// \return more readable error string
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

    /// Function used to select openCL GPU device
    /// \param desired_gpu_device name of the GPU that should be selected,
    ///                         if default name then default device is selected
    /// \return selected openCL GPU device
    static cl::Device select_gpu(const std::string &desired_gpu_device);

    /// This function prints out build logs from building kernels of the implemented openCL program interface
    /// \param program program interface used to build kernels
    /// \return error value if something during compilation failed
    static cl_int dump_build_log(cl::Program& program);
};


#endif //OCL_TEST_OPENCLCOMPONENT_H

//
// Created by pulta on 27.11.2023.
//

#include "OpenCLComponent.h"
#include "../CalculationScheduler.h"

#include <utility>
#include <vector>
#include <iostream>

#define CL_HPP_ENABLE_EXCEPTIONS
#include <CL/opencl.hpp>


#ifdef _MSC_VER
#pragma comment(lib, "opencl.lib")
#endif

const char* full_correlation_kernel_name = "FULL_CORRELATION_KERNEL";
const char* full_correlation_kernel_source = R"(
#pragma OPENCL EXTENSION cl_khr_fp64 : enable

__kernel void FULL_CORRELATION_KERNEL(__constant double *acc_x,
                                 __constant double *acc_y,
                                 __constant double *acc_z,
                                 __constant double *hr_values,
                                 __global double* c,
                                 __global unsigned char* p,
                                 __local double *local_sum_acc,
                                 __local double *local_sum_acc2,
                                 __local double *local_sum_acc_hr,
                                 __global double *result_acc,
                                 __global double *result_acc2,
                                 __global double *result_acc_hr) {

    size_t global_id = get_global_id(0);
	size_t local_id = get_local_id(0);
    size_t group_id = get_group_id(0);
    size_t group_size = get_local_size(0);
	unsigned int stride = 1;

	double x = acc_x[global_id];
	double y = acc_y[global_id];
	double z = acc_z[global_id];

	double acc = c[0] * pow(x, p[0]) + c[1] * pow(y, p[1])
                    + c[2] * pow(z, p[2]) + c[3];

	double hr = hr_values[global_id];
	local_sum_acc[local_id] = acc;
	local_sum_acc2[local_id] = acc * acc;
	local_sum_acc_hr[local_id] = acc * hr;

    barrier(CLK_LOCAL_MEM_FENCE);

    for(int i = group_size/2; i > 0; i >>= 1) {
        if(local_id < i) {
			local_sum_acc[local_id] += local_sum_acc[local_id + i];
			local_sum_acc2[local_id] += local_sum_acc2[local_id + i];
			local_sum_acc_hr[local_id] += local_sum_acc_hr[local_id + i];
        }
        barrier(CLK_LOCAL_MEM_FENCE);
    }

    if (local_id == 0) {
		result_acc[group_id] = local_sum_acc[0];
		result_acc2[group_id] = local_sum_acc2[0];
		result_acc_hr[group_id] = local_sum_acc_hr[0];
	}
}
)";

void OpenCLComponent::init_opencl_device(std::unique_ptr<OpenCLComponent> &cl_device,
                                         const std::string& desired_gpu_device) {
#if defined(CL_HPP_ENABLE_EXCEPTIONS)
    cl::Program program;
    try {
#endif

        auto selected_device = select_gpu(desired_gpu_device);

        //select all needed source codes
        std::vector<std::string> source_codes{full_correlation_kernel_source};
        const cl::Program::Sources& sources(source_codes);
        // get program interface for sources and device context
        cl::Context device_context = cl::Context(selected_device);
        program = cl::Program(device_context, sources);

        program.build(selected_device, "-cl-std=CL2.0 -cl-denorms-are-zero");

        cl::Kernel full_correlation_kernel = cl::Kernel(program, full_correlation_kernel_name);
        auto build_err = dump_build_log(program);
        if(build_err != CL_SUCCESS){

            std::cerr << "Error occurred during initialization: " << build_err << "("
                      << Get_OpenCL_Error_Desc(build_err) << ")" << std::endl;
            return;
        }
        size_t work_group_size = full_correlation_kernel
                                    .getWorkGroupInfo<CL_KERNEL_WORK_GROUP_SIZE>(selected_device);
        size_t max_work_group_size = selected_device
                                    .getInfo<CL_DEVICE_MAX_WORK_GROUP_SIZE>();

        std::cout << "Workgroup size: " << work_group_size << std::endl;
        std::cout << "Max workgroup size: " << max_work_group_size << std::endl;

    	cl_device = std::make_unique<OpenCLComponent>(selected_device, device_context,
                                                      full_correlation_kernel,
                                                      max_work_group_size);

#if defined(CL_HPP_ENABLE_EXCEPTIONS)
    } catch (cl::Error &err) {
        std::cerr << "Error occurred during initialization: " << err.what() << "(" << err.err() << " - "
                  << Get_OpenCL_Error_Desc(err.err()) << ")" << std::endl;
        dump_build_log(program);
        exit(1);
    }
#endif
}

OpenCLComponent::OpenCLComponent(cl::Device selected_device, cl::Context device_context,
                                 cl::Kernel full_corr_kernel, size_t work_group_size):

                                 selected_device(std::move(selected_device)),
                                 device_context(std::move(device_context)),
                                 full_corr_kernel(std::move(full_corr_kernel)),
                                 work_group_size(work_group_size){

}


OpenCLComponent::~OpenCLComponent() = default;

const char* OpenCLComponent::Get_OpenCL_Error_Desc(cl_int error) noexcept{
    switch (error){
        case 0: return "CL_SUCCESS";
        case -1: return "CL_DEVICE_NOT_FOUND";
        case -2: return "CL_DEVICE_NOT_AVAILABLE";
        case -3: return "CL_COMPILER_NOT_AVAILABLE";
        case -4: return "CL_MEM_OBJECT_ALLOCATION_FAILURE";
        case -5: return "CL_OUT_OF_RESOURCES";
        case -6: return "CL_OUT_OF_HOST_MEMORY";
        case -7: return "CL_PROFILING_INFO_NOT_AVAILABLE";
        case -8: return "CL_MEM_COPY_OVERLAP";
        case -9: return "CL_IMAGE_FORMAT_MISMATCH";
        case -10: return "CL_IMAGE_FORMAT_NOT_SUPPORTED";
        case -11: return "CL_BUILD_PROGRAM_FAILURE";
        case -12: return "CL_MAP_FAILURE";
        case -13: return "CL_MISALIGNED_SUB_BUFFER_OFFSET";
        case -14: return "CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST";
        case -15: return "CL_COMPILE_PROGRAM_FAILURE";
        case -16: return "CL_LINKER_NOT_AVAILABLE";
        case -17: return "CL_LINK_PROGRAM_FAILURE";
        case -18: return "CL_DEVICE_PARTITION_FAILED";
        case -19: return "CL_KERNEL_ARG_INFO_NOT_AVAILABLE";

            // compile-time errors
        case -30: return "CL_INVALID_VALUE";
        case -31: return "CL_INVALID_DEVICE_TYPE";
        case -32: return "CL_INVALID_PLATFORM";
        case -33: return "CL_INVALID_DEVICE";
        case -34: return "CL_INVALID_CONTEXT";
        case -35: return "CL_INVALID_QUEUE_PROPERTIES";
        case -36: return "CL_INVALID_COMMAND_QUEUE";
        case -37: return "CL_INVALID_HOST_PTR";
        case -38: return "CL_INVALID_MEM_OBJECT";
        case -39: return "CL_INVALID_IMAGE_FORMAT_DESCRIPTOR";
        case -40: return "CL_INVALID_IMAGE_SIZE";
        case -41: return "CL_INVALID_SAMPLER";
        case -42: return "CL_INVALID_BINARY";
        case -43: return "CL_INVALID_BUILD_OPTIONS";
        case -44: return "CL_INVALID_PROGRAM";
        case -45: return "CL_INVALID_PROGRAM_EXECUTABLE";
        case -46: return "CL_INVALID_KERNEL_NAME";
        case -47: return "CL_INVALID_KERNEL_DEFINITION";
        case -48: return "CL_INVALID_KERNEL";
        case -49: return "CL_INVALID_ARG_INDEX";
        case -50: return "CL_INVALID_ARG_VALUE";
        case -51: return "CL_INVALID_ARG_SIZE";
        case -52: return "CL_INVALID_KERNEL_ARGS";
        case -53: return "CL_INVALID_WORK_DIMENSION";
        case -54: return "CL_INVALID_WORK_GROUP_SIZE";
        case -55: return "CL_INVALID_WORK_ITEM_SIZE";
        case -56: return "CL_INVALID_GLOBAL_OFFSET";
        case -57: return "CL_INVALID_EVENT_WAIT_LIST";
        case -58: return "CL_INVALID_EVENT";
        case -59: return "CL_INVALID_OPERATION";
        case -60: return "CL_INVALID_GL_OBJECT";
        case -61: return "CL_INVALID_BUFFER_SIZE";
        case -62: return "CL_INVALID_MIP_LEVEL";
        case -63: return "CL_INVALID_GLOBAL_WORK_SIZE";
        case -64: return "CL_INVALID_PROPERTY";
        case -65: return "CL_INVALID_IMAGE_DESCRIPTOR";
        case -66: return "CL_INVALID_COMPILER_OPTIONS";
        case -67: return "CL_INVALID_LINKER_OPTIONS";
        case -68: return "CL_INVALID_DEVICE_PARTITION_COUNT";

            // extension errors
        case -1000: return "CL_INVALID_GL_SHAREGROUP_REFERENCE_KHR";
        case -1001: return "CL_PLATFORM_NOT_FOUND_KHR";
        case -1002: return "CL_INVALID_D3D10_DEVICE_KHR";
        case -1003: return "CL_INVALID_D3D10_RESOURCE_KHR";
        case -1004: return "CL_D3D10_RESOURCE_ALREADY_ACQUIRED_KHR";
        case -1005: return "CL_D3D10_RESOURCE_NOT_ACQUIRED_KHR";

        default: return "Unknown OpenCL error";
    }
}

cl_int OpenCLComponent::dump_build_log(cl::Program& program) {
    cl_int build_err = CL_SUCCESS;
    auto buildInfo = program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(&build_err);
    bool printed_logo = false;

    if (!buildInfo.empty()) {
        for (auto& pair : buildInfo) {
            const auto msg = pair.second;
            if (!msg.empty()) {
                if (!printed_logo) {
                    std::wcout << L"Errors messages during initialization:" << std::endl;
                    printed_logo = true;
                }
                std::cout << msg << std::endl << std::endl;
            }
        }
    }
    return build_err;
}

cl::Device OpenCLComponent::select_gpu(const std::string &desired_gpu_device) {
    std::vector<cl::Platform> platforms;
    cl::Platform::get(&platforms);

    for (auto& platform : platforms) {
        std::vector<cl::Device> devices;
        platform.getDevices(CL_DEVICE_TYPE_GPU, &devices);

        for (auto& device : devices) {
            const auto is_available = device.getInfo<CL_DEVICE_AVAILABLE>();
            const auto desc = device.getInfo<CL_DEVICE_NAME>();
            if(is_available){
                if(desired_gpu_device == DEFAULT_GPU_NAME || desc == desired_gpu_device){
                    std::cout << "Selected GPU Device " << desc << " on platform "
                            << platform.getInfo<CL_PLATFORM_NAME>() << std::endl;
                    return device;
                }
            }else{
                std::cout << "Found unavailable device " << desc << " on platform "
                                << platform.getInfo<CL_PLATFORM_NAME>() << std::endl;
            }
        }
    }

    if(desired_gpu_device != DEFAULT_GPU_NAME){
        std::cerr << "There is no gpu device with name" << desired_gpu_device << " found!";
        exit(-1);
    }

    cl::Platform platform = cl::Platform::getDefault();
    cl::Device device = cl::Device::getDefault();
    auto desc = device.getInfo<CL_DEVICE_NAME>();
    std::cout << "Selected default gpu device " << desc << " on platform "
                                    << platform.getInfo<CL_PLATFORM_NAME>() << std::endl;
    return device;
}


void OpenCLComponent::calculate_correlation(const genome &curr_gen, std::array<std::vector<double>, 3>& out_sums,
                                            const cl::size_type entries_count, const size_t work_groups_count,
                                            cl::Event &corr_kernel_event) {

    cl::CommandQueue cmd_queue(this->device_context, this->selected_device);

    // first init function specific buffers
    cl::Buffer out_sum_acc_buff = cl::Buffer(this->device_context,
                                             CL_MEM_WRITE_ONLY | CL_MEM_HOST_READ_ONLY,
                                        work_groups_count * sizeof(double), nullptr);

    cl::Buffer out_sum_acc2_buff = cl::Buffer(this->device_context,
                                              CL_MEM_WRITE_ONLY | CL_MEM_HOST_READ_ONLY,
                                         work_groups_count * sizeof(double), nullptr);

    cl::Buffer out_sum_acc_hr_buff = cl::Buffer(this->device_context,
                                                CL_MEM_WRITE_ONLY | CL_MEM_HOST_READ_ONLY,
                                           work_groups_count * sizeof(double), nullptr);

    auto constants = cl::Buffer(this->device_context,
                                CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR | CL_MEM_HOST_NO_ACCESS,
                                GENOME_CONSTANTS_SIZE * sizeof(double),
                                const_cast<double *>(curr_gen.constants.data()));

    auto powers = cl::Buffer(this->device_context,
                             CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR | CL_MEM_HOST_NO_ACCESS,
                             GENOME_POW_SIZE * sizeof(uint8_t),
                             const_cast<unsigned char *>(curr_gen.powers.data()));

    //assign to kernel arguments
    this->full_corr_kernel.setArg(4, constants);
    this->full_corr_kernel.setArg(5, powers);

    this->full_corr_kernel.setArg(6, work_group_size * sizeof(double), nullptr);
    this->full_corr_kernel.setArg(7, work_group_size * sizeof(double), nullptr);
    this->full_corr_kernel.setArg(8, work_group_size * sizeof(double), nullptr);

    this->full_corr_kernel.setArg(9, out_sum_acc_buff);
    this->full_corr_kernel.setArg(10, out_sum_acc2_buff);
    this->full_corr_kernel.setArg(11, out_sum_acc_hr_buff);
    
    // Create a command queue to communicate with the OpenCL device.
    cmd_queue.enqueueNDRangeKernel(this->full_corr_kernel, cl::NullRange,
                                    cl::NDRange(entries_count),
									cl::NDRange(this->work_group_size),
                                   nullptr, &corr_kernel_event);

    // Read the results from the OpenCL device.
    cmd_queue.enqueueReadBuffer(out_sum_acc_buff, CL_FALSE, 0,
                                 work_groups_count * sizeof(double), out_sums[0].data());
    cmd_queue.enqueueReadBuffer(out_sum_acc2_buff, CL_FALSE, 0,
                                 work_groups_count * sizeof(double), out_sums[1].data());
    cmd_queue.enqueueReadBuffer(out_sum_acc_hr_buff, CL_FALSE, 0,
                                 work_groups_count * sizeof(double), out_sums[2].data());
}

void OpenCLComponent::init_static_buffers(const std::shared_ptr<input_data> &input, size_t numbers_bytes_size,
                                          const size_t work_groups_count) {
    if(buffers_initialized){
        return;
    }
    this->x_acc_vector = cl::Buffer(this->device_context,
                                  CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR | CL_MEM_HOST_NO_ACCESS,
                                                numbers_bytes_size, input->acc_x->values.data());

    this->full_corr_kernel.setArg(0, this->x_acc_vector);


    this->y_acc_vector = cl::Buffer(this->device_context,
                                  CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR | CL_MEM_HOST_NO_ACCESS,
                                                numbers_bytes_size, input->acc_y->values.data());
    this->full_corr_kernel.setArg(1, this->y_acc_vector);


    this->z_acc_vector = cl::Buffer(this->device_context,
                                  CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR | CL_MEM_HOST_NO_ACCESS,
                                                numbers_bytes_size, input->acc_z->values.data());
    this->full_corr_kernel.setArg(2, this->z_acc_vector);

    this->hr_vector = cl::Buffer(this->device_context,
                                 CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR | CL_MEM_HOST_NO_ACCESS,
                                 numbers_bytes_size, input->hr->values.data());
    this->full_corr_kernel.setArg(3, this->hr_vector);

    buffers_initialized = true;
}

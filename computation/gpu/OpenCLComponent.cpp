//
// Created by pulta on 27.11.2023.
//

#include "OpenCLComponent.h"
#include "../CalculationScheduler.h"

#include <vector>
#include <iostream>

#define CL_HPP_ENABLE_EXCEPTIONS
#define CL_HPP_TARGET_OPENCL_VERSION 200
#include <CL/opencl.hpp>


#ifdef _MSC_VER
#pragma comment(lib, "opencl.lib")
#endif


const char* transform_kernel_name = "TRANSFORM_KERNEL";
//todo __constant or __local?
//todo workgroups
//todo bigger arrays wont fit in global_id probably
const char* transform_kernel_source = R"(
__kernel void TRANSFORM_KERNEL( __global double *acc_x,
                                __global double *acc_y,
                                __global double *acc_z,
                                __constant double* c,
                                __constant unsigned char* p,
                                __global double *result) {

    unsigned int global_id = get_global_id(0);

	double x = acc_x[global_id];
	double y = acc_y[global_id];
	double z = acc_z[global_id];

	result[global_id] = c[0] * pow(x, p[0]) + c[1] * pow(y, p[1])
                + c[2] * pow(z, p[2]) + c[3];
}
)";
const char* correlation_kernel_name = "CORRELATION_KERNEL";
//todo bigger arrays wont fit in global_id probably
const char* correlation_kernel_source = R"(
__kernel void CORRELATION_KERNEL(__global double *acc_trs_values,
                                 __global double *hr_values,
                                 __local double* local_sum_acc,
                                 __local double *local_sum_acc2,
                                 __local double *local_sum_acc_hr,
                                 __global double *result_acc,
                                 __global double *result_acc2,
                                 __global double *result_acc_hr) {

    size_t global_id = get_global_id(0);
	size_t local_id = get_local_id(0);
    size_t group_id = get_group_id(0);
    size_t local_size = get_local_size(0);
	unsigned int stride = 1;

	double acc = acc_trs_values[global_id];
	double hr = hr_values[global_id];
	local_sum_acc[local_id] = acc;
	local_sum_acc2[local_id] = acc * acc;
	local_sum_acc_hr[local_id] = acc * hr;

	do {
		unsigned int next_stride = 2 * stride;
		unsigned int neighbor_idx = local_id + stride;
		if ((local_id % next_stride) == 0) {
			local_sum_acc[local_id] = local_sum_acc[local_id] + local_sum_acc[neighbor_idx];
			local_sum_acc2[local_id] = local_sum_acc2[local_id] + local_sum_acc2[neighbor_idx];
			local_sum_acc_hr[local_id] = local_sum_acc_hr[local_id] + local_sum_acc_hr[neighbor_idx];
		}

		stride = next_stride;

		barrier(CLK_LOCAL_MEM_FENCE);
	} while (stride < local_size);


    if (local_id == 0) {
		result_acc[group_id] = local_sum_acc[0];
		result_acc2[group_id] = local_sum_acc2[0];
		result_acc_hr[group_id] = local_sum_acc_hr[0];
	}
}
)";
OpenCLComponent::OpenCLComponent() {
    cl::Program program;
#if defined(CL_HPP_ENABLE_EXCEPTIONS)
    try {
#endif
        this->selected_device = try_select_first_gpu();//todo let the user choose gpu somehow

        std::vector<std::string> source_codes{transform_kernel_source, correlation_kernel_source};
        const cl::Program::Sources& sources(source_codes);
        this->device_context = std::make_unique<cl::Context>(this->selected_device);
        program = (device_context, sources);

        program.build(this->selected_device, "-cl-std=CL2.0 -cl-denorms-are-zero");
        this->transform_kernel = std::make_unique<cl::Kernel>(program, transform_kernel_name);
        this->correlation_kernel = std::make_unique<cl::Kernel>(program, correlation_kernel_name);
        dump_build_log(program);

        this->trans_queue = std::make_unique<cl::CommandQueue>(*this->device_context, this->selected_device);
        this->corr_queue = std::make_unique<cl::CommandQueue>(*this->device_context, this->selected_device);

        this->work_group_size = this->correlation_kernel
                            ->getWorkGroupInfo<CL_KERNEL_WORK_GROUP_SIZE>(this->selected_device);

#if defined(CL_HPP_ENABLE_EXCEPTIONS)
    } catch (cl::Error err) {
        std::cerr << "Error occurred during initialization: " << err.what() << "(" << err.err() << " - "
        << Get_OpenCL_Error_Desc(err.err()) << ")" << std::endl;
        dump_build_log(program);
        this->build_err = CL_INVALID_VALUE;
    }
#endif
}


OpenCLComponent::~OpenCLComponent() {
    //todo
}

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

void OpenCLComponent::dump_build_log(cl::Program& program) {
    this->build_err = CL_SUCCESS;
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
}

cl::Device OpenCLComponent::try_select_first_gpu() {
    std::vector<cl::Platform> platforms;
    cl::Platform::get(&platforms);

    for (auto& platform : platforms) {
        std::vector<cl::Device> devices;
        platform.getDevices(CL_DEVICE_TYPE_GPU, &devices);

        for (auto& device : devices) {
            const auto is_available = device.getInfo<CL_DEVICE_AVAILABLE>();
            const auto desc = device.getInfo<CL_DEVICE_NAME>();
            if(is_available){
                std::cout << "Vybrano zarizeni " << desc << " na platforme "
                                << platform.getInfo<CL_PLATFORM_NAME>() << std::endl;
                snitch_device_info(device);
                return device;
            }else{
                std::cout << "Nedostupne zarizeni " << desc << " na platforme "
                                << platform.getInfo<CL_PLATFORM_NAME>() << std::endl;
            }
        }
    }

    cl::Platform platform = cl::Platform::getDefault();
    cl::Device device = cl::Device::getDefault();
    auto desc = device.getInfo<CL_DEVICE_NAME>();
    std::cout << "Vybrano vychozi zarizeni " << desc << " na platforme " << platform.getInfo<CL_PLATFORM_NAME>() << std::endl;
    snitch_device_info(device);
    return device;
}

bool OpenCLComponent::was_init_successful() const {
    return this->build_err == CL_SUCCESS;
}

void
OpenCLComponent::calculate_correlation(const genome &curr_gen, std::vector<double> &out_sum_acc,
                                       std::vector<double> &out_sum_acc2, std::vector<double> &out_sum_acc_hr,
                                       const cl::size_type numbers_bytes_size) {

    const auto work_groups_count = numbers_bytes_size / this->work_group_size;
    cl::vector<cl::Event> trans_kernel_event(1);	//async call
    cl::Event corr_kernel_event{};
    transform_acc(numbers_bytes_size, curr_gen, trans_kernel_event[0]);

    auto kernel = *this->device_context;

    cl::Buffer out_sum_acc_buff(*this->device_context, CL_MEM_WRITE_ONLY | CL_MEM_HOST_READ_ONLY,
                                work_groups_count, nullptr);

    cl::Buffer out_sum_acc2_buff(*this->device_context, CL_MEM_WRITE_ONLY | CL_MEM_HOST_READ_ONLY,
                                 work_groups_count, nullptr);

    cl::Buffer out_sum_acc_hr_buff(*this->device_context, CL_MEM_WRITE_ONLY | CL_MEM_HOST_READ_ONLY,
                                   work_groups_count, nullptr);


    this->correlation_kernel->setArg(3, work_group_size * sizeof(double), nullptr);
    this->correlation_kernel->setArg(4, work_group_size * sizeof(double), nullptr);
    this->correlation_kernel->setArg(5, work_group_size * sizeof(double), nullptr);

    this->correlation_kernel->setArg(6, out_sum_acc_buff);
    this->correlation_kernel->setArg(7, out_sum_acc2_buff);
    this->correlation_kernel->setArg(8, out_sum_acc_hr_buff);
    //todo sus address passing
    this->corr_queue->enqueueNDRangeKernel(*this->correlation_kernel, cl::NullRange,
                                    cl::NDRange(numbers_bytes_size), cl::NullRange,
                                    &trans_kernel_event, &corr_kernel_event);

    // Read the results from the OpenCL device.
    this->corr_queue->enqueueReadBuffer(out_sum_acc_buff, CL_TRUE, 0,
                                 work_groups_count * sizeof(double), out_sum_acc.data());
    this->corr_queue->enqueueReadBuffer(out_sum_acc2_buff, CL_TRUE, 0,
                                 work_groups_count * sizeof(double), out_sum_acc2.data());
    this->corr_queue->enqueueReadBuffer(out_sum_acc_hr_buff, CL_TRUE, 0,
                                 work_groups_count * sizeof(double), out_sum_acc_hr.data());

    // corr_kernel_event.wait() not needed
}

void OpenCLComponent::transform_acc(const cl::size_type numbers_bytes_size, const genome& current_gen,
                                    cl::Event& trans_event) {

    auto kernel = *this->device_context;

    auto constants = cl::Buffer(*this->device_context,
                                CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR | CL_MEM_HOST_NO_ACCESS,
                             GENOME_CONSTANTS_SIZE * sizeof(double),
                             const_cast<double *>(current_gen.constants.data()));

    auto powers = cl::Buffer(*this->device_context,
                             CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR | CL_MEM_HOST_NO_ACCESS,
                             GENOME_POW_SIZE * sizeof(uint8_t),
                             const_cast<unsigned char *>(current_gen.powers.data()));

    this->transform_kernel->setArg(3, constants);
    this->transform_kernel->setArg(4, powers);

    this->trans_queue->enqueueNDRangeKernel(*this->transform_kernel, cl::NullRange,
                               cl::NDRange(numbers_bytes_size), cl::NullRange,
                                   nullptr, &trans_event);

    //todo unmap in destructor if possible queue.enqueueUnmapMemObject(result_cl, result);
}

void OpenCLComponent::init_static_buffers(const std::shared_ptr<input_data> &input, size_t numbers_bytes_size) {
    if(buffers_initialized){
        return;
    }
    if(this->x_acc_vector == nullptr){
        this->x_acc_vector = std::make_unique<cl::Buffer>(*this->device_context,
                                      CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR | CL_MEM_HOST_NO_ACCESS,
                                                    numbers_bytes_size, input->acc_x->values.data());

//        this->trans_queue->enqueueWriteBuffer(*this->x_acc_vector, CL_TRUE, 0, numbers_bytes_size,
//                                       input->acc_x->values.data(), nullptr, nullptr);	//blocking call
        this->transform_kernel->setArg(0, this->x_acc_vector.get());

    }

    if(this->y_acc_vector == nullptr) {
        this->y_acc_vector = std::make_unique<cl::Buffer>(*this->device_context,
                                      CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR | CL_MEM_HOST_NO_ACCESS,
                                                    numbers_bytes_size, input->acc_y->values.data());
//        this->trans_queue->enqueueWriteBuffer(*this->y_acc_vector, CL_TRUE, 0, numbers_bytes_size,
//                                       input->acc_y->values.data(), nullptr, nullptr);	//blocking call
        this->transform_kernel->setArg(1, this->y_acc_vector.get());
    }

    if(this->z_acc_vector == nullptr) {
        this->z_acc_vector = std::make_unique<cl::Buffer>(*this->device_context,
                                      CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR | CL_MEM_HOST_NO_ACCESS,
                                                    numbers_bytes_size, input->acc_z->values.data());
//        this->trans_queue->enqueueWriteBuffer(*this->z_acc_vector, CL_TRUE, 0, numbers_bytes_size,
//                                       input->acc_z->values.data(), nullptr, nullptr);	//blocking call
        this->transform_kernel->setArg(2, this->z_acc_vector.get());
    }

    if(this->transform_result == nullptr){
        //todo enqueueWriteBuffer enqueueUnmap? + other buffers?
        this->transform_result = std::make_unique<cl::Buffer>(cl::Buffer(*this->device_context,
                                                                   CL_MEM_READ_WRITE | CL_MEM_HOST_NO_ACCESS,
                                                                   numbers_bytes_size, nullptr));
        this->transform_kernel->setArg(5, this->transform_result.get());
        this->correlation_kernel->setArg(0, this->transform_result.get());
    }

    if(this->hr_vector == nullptr){
        this->hr_vector = std::make_unique<cl::Buffer>(*this->device_context,
                               CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR | CL_MEM_HOST_NO_ACCESS,
                               numbers_bytes_size, input->hr->values.data());
        //todo enqueueMapSVM??
//        this->corr_queue->enqueueWriteBuffer(*this->hr_vector, CL_TRUE, 0, numbers_bytes_size,
//                                       input->hr->values.data(), nullptr, nullptr);	//blocking call

        this->correlation_kernel->setArg(1, this->hr_vector.get());
    }

    buffers_initialized = true;
}

void OpenCLComponent::snitch_device_info(cl::Device &device) {

//    auto compute_units = device.getInfo<CL_DEVICE_MAX_COMPUTE_UNITS>();
//    auto work_itm_dim = device.getInfo<CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS>();
//    auto max_work_group_size = device.getInfo<CL_DEVICE_MAX_WORK_GROUP_SIZE>();
//    auto max_work_itm_size = device.getInfo<CL_DEVICE_MAX_WORK_ITEM_SIZES>();
//    auto pref_vector_width = device.getInfo<CL_DEVICE_PREFERRED_VECTOR_WIDTH_DOUBLE>();
//    auto max_mem_alloc_size = device.getInfo<CL_DEVICE_MAX_MEM_ALLOC_SIZE>();
//    auto max_param_size = device.getInfo<CL_DEVICE_MAX_PARAMETER_SIZE>();
    this->global_mem_size = device.getInfo<CL_DEVICE_GLOBAL_MEM_SIZE>();
    this->local_mem_size = device.getInfo<CL_DEVICE_LOCAL_MEM_SIZE>();
}

//
// Created by pulta on 23.10.2023.
//

#include <fstream>
#include <iostream>
#include <sstream>
#include <ppl.h>
#include <limits>
#include <filesystem>
#include <stack>
#include <utility>
#include <execution>
#include "Preprocessor.h"
#include "../utils.h"


bool Preprocessor::load_and_preprocess(std::string &hr_file, std::string &acc_file,
                                       const std::shared_ptr<input_data> &result,
                                       const std::string &folder_name) const {

    bool files_ok = analyze_files(hr_file, acc_file, result);
    if(!files_ok){
        return false;
    }
    std::cout << "Files analyzed under folder " << folder_name <<
                ". Sampling rate of ACC is " << (int)result->sampling_rate << std::endl;

    const auto current_offset = result->hr_entries_count;

    process_file_content(hr_file, false, result);
    process_file_content(acc_file, true, result);

    preprocess_acc_vectors(result, current_offset);
    return true;
}

void Preprocessor::preprocess_acc_vectors(const std::shared_ptr<input_data> &result, size_t current_offset) const {
    auto& x_input = result->acc_x->values;
    auto& y_input = result->acc_y->values;
    auto& z_input = result->acc_z->values;
    const uint8_t sampling_rate = result->sampling_rate;

    //are there are more entries in the hr vector than the acc vector?
    if(result->acc_entries_count < result->hr_entries_count){
        result->hr_entries_count = result->acc_entries_count;
    }

    const auto count = result->acc_entries_count;

    #pragma loop(no_vector)
    for(size_t i = current_offset; i < count; ++i){
        x_input[i] /= sampling_rate;
        y_input[i] /= sampling_rate;
        z_input[i] /= sampling_rate;
    }
}

bool Preprocessor::load_and_preprocess_folder(const std::shared_ptr<input_data> &result){
    directory_map directories {};
    collect_directory_data_files_entries(directories);
    std::cout << "Loaded total of " << directories.size() << " directories!" << std::endl;
    if(directories.empty()){
        return false;
    }

    //init all vectors according to all files size
    result->hr = std::make_unique<input_vector>(this->total_predicted_input_size);
    result->acc_x = std::make_unique<input_vector>(this->total_predicted_input_size);
    result->acc_y = std::make_unique<input_vector>(this->total_predicted_input_size);
    result->acc_z = std::make_unique<input_vector>(this->total_predicted_input_size);

    for (const auto& data_entry: directories) {
        std::string hr_file = data_entry.second.first;
        std::string acc_file = data_entry.second.second;
        //preprocess hr and acc file pair and save to vectors
        if(!load_and_preprocess(hr_file, acc_file, result, data_entry.first)){
            return false;
        }
    }

    if(result->hr_entries_count <= 0){
        return false;
    }

    __int64 elapsed = time_call([&] {
        //after all files are loaded normalization is done + calculate sums that can be calculated one time
        calculate_hr_init_data(result);

        post_process(result);
    });
    std::cout << "Normalization of input data took " << elapsed << " ms" << std::endl;

    result->hr->values.resize(result->hr_entries_count);
    result->acc_x->values.resize(result->acc_entries_count);
    result->acc_y->values.resize(result->acc_entries_count);
    result->acc_z->values.resize(result->acc_entries_count);

    print_input_data_statistics(result);
    return true;
}

void Preprocessor::print_input_data_statistics(const std::shared_ptr<input_data> &result) {
    std::cout << TEXT_SEPARATOR << std::endl;
    std::cout << "Statistics after data load:" << std::endl;
    std::cout << "Entries count: " << result->hr_entries_count << std::endl;
    std::cout << "HR sum: " << result->hr_sum << std::endl;
    std::cout << "HR squared sum: " << result->squared_hr_corr_sum << std::endl;
    std::cout << "HR min/max: " << result->hr->min << "/" << result->hr->max << std::endl;
    std::cout << "ACCX min/max: " << result->acc_x->min << "/" << result->acc_x->max << std::endl;
    std::cout << "ACCY min/max: " << result->acc_y->min << "/" << result->acc_y->max << std::endl;
    std::cout << "ACCZ min/max: " << result->acc_z->min << "/" << result->acc_z->max << std::endl;
}


file_type is_data_file(const std::string& file_name){
    if(file_name.compare(0, 3, "HR_") == 0){
        return file_type::HR_DATA_FILE;
    }else if(file_name.compare(0, 4, "ACC_") == 0){
        return file_type::ACC_DATA_FILE;
    }
    return file_type::NOT_DATA_FILE;
}

void Preprocessor::collect_directory_data_files_entries(directory_map& folder_map) {
    std::stack<std::filesystem::path> path_stack;
    path_stack.emplace(this->input_folder);

    while (!path_stack.empty()) {
        std::filesystem::path currentPath = path_stack.top();
        path_stack.pop();

        for (const auto& entry: std::filesystem::directory_iterator(currentPath)) {
            if (entry.is_directory()) {
                path_stack.push(entry.path());
            } else if (entry.is_regular_file()) {
                const auto& file_path = entry.path();
                auto parent_folder_name = file_path.parent_path().filename().string();
                auto file_type = is_data_file(file_path.filename().string());
                if(file_type == file_type::NOT_DATA_FILE){
                    continue;
                }
                if(folder_map.find(parent_folder_name) == folder_map.end()){
                    folder_map[parent_folder_name] = std::make_pair("", "");
                }
                auto file_path_string = file_path.string();
                switch (file_type) {
                    case HR_DATA_FILE:
                        folder_map[parent_folder_name].first = file_path_string;
                        this->total_predicted_input_size += get_data_vector_needed_size(file_path_string);
                        break;
                    case ACC_DATA_FILE:
                        folder_map[parent_folder_name].second = file_path_string;
                        break;
                    default:
                        std::cerr << "Unknown error occurred during directory traversing." << std::endl;
                }
            }
        }
    }
}

size_t Preprocessor::get_data_vector_needed_size(const std::string& input_file){

    auto file_size = std::filesystem::file_size(input_file);

    const size_t line_char_count = 24; //there are 24-25 characters per line typically

    size_t vector_size = file_size / line_char_count;
    //cut it so it can be easily vectorized
    vector_size -= vector_size % VECTOR_SIZE;

    return vector_size;
}

void Preprocessor::process_file_content(const std::string &input_file, bool is_acc_file,
                                        const std::shared_ptr<input_data> &result) const{
    std::ifstream file(input_file);
    if (file.is_open()) {

        __int64 elapsed = time_call([&] {
            if (is_acc_file) {
                load_acc_file_content(result, file);
            } else {
                load_hr_file_content(result, file);
            }
        });
        file.close();
        std::cout << "Parsing of file (" << input_file << ") took " << elapsed << " ms" << std::endl;

    } else {
        std::cerr << "Error: Unable to open the file." << std::endl;
        exit(-1);
    }
}

void Preprocessor::load_hr_file_content(const std::shared_ptr<input_data> &result, std::ifstream &file) const {
    auto& hr_input = result->hr->values;
    std::string line;

    skip_past_time_entries(result->first_acc_time, result->hr_date_end_index, file, line);

    const size_t offset = result->hr_entries_count;
    const size_t data_start_index = result->hr_date_end_index + 1;

    size_t count = 0;
    do {
        // auto date = line.substr(0, date_end_index);
        auto hr = line.substr(data_start_index, 6); //6 chars is the maximum possible size of hr entry
        hr_input[offset + count] = std::stod(hr);
        ++count;

    } while (std::getline(file, line));
    result->hr_entries_count += count - count % VECTOR_SIZE;
    std::cout << "Loaded " << count << " entries from hr file" << std::endl;
}

void Preprocessor::load_acc_file_content(const std::shared_ptr<input_data> &result, std::ifstream &file) const {
    auto& x_input = result->acc_x->values;
    auto& y_input = result->acc_y->values;
    auto& z_input = result->acc_z->values;
    std::string line;
    const size_t offset = result->acc_entries_count;

    std::getline(file, line); //skip first line (header)

    //first filter all lines that are before our wanted time
    skip_past_time_entries(result->first_hr_time, result->acc_date_end_index, file, line);

    //now we can start saving some data
    const size_t data_start_index = result->acc_date_end_index + 1;
    const uint8_t sampling_rate = result->sampling_rate;
    const size_t entries_count = result->hr_entries_count - offset;
    uint8_t count = 0;
    size_t index = 0;
    do{
//        auto date = line.substr(0, date_end_index);
        auto x_value_end = line.find(',', data_start_index);
        auto y_value_end = line.find(',', x_value_end + 1);

        auto x = line.substr(data_start_index, x_value_end - data_start_index);
        auto y = line.substr(x_value_end + 1, y_value_end - x_value_end - 1);
        auto z = line.substr(y_value_end + 1, line.size() - y_value_end - 1);

        x_input[offset + index] += std::stod(x);
        y_input[offset + index] += std::stod(y);
        z_input[offset + index] += std::stod(z);
        ++count;
        if(count >= sampling_rate){
            count = 0;
            ++index;
            if(index >= entries_count){
                break;
            }
        }
    } while(std::getline(file, line));

    result->acc_entries_count += index - index % VECTOR_SIZE;
    std::cout << "Loaded " << count << " entries from acc file" << std::endl;
}

void Preprocessor::skip_past_time_entries(const time_t begin_time, const size_t date_end_index, std::ifstream &file,
                                          std::string &result_line) {
    std::string line;
    while (std::getline(file, line)) { 
        auto date = line.substr(0, date_end_index);
        auto time = parse_date(date);
        if(time < begin_time){
            continue;
        }else{
            result_line = line;
            break;
        }
    }
}

inline void Preprocessor::norm_input_vector(const std::unique_ptr<input_vector>& vector) const{
    auto max = vector->max;
    auto min = vector->min;
    auto& arr = vector->values;
    const auto scope = (max - min);

//    #pragma loop(no_vector)
    for(double& value : arr){
        value = (value - min) / scope;
    }
}

void Preprocessor::post_process(const std::shared_ptr<input_data> &data) const {
    find_min_max(data->acc_x);
    find_min_max(data->acc_y);
    find_min_max(data->acc_z);
    find_min_max(data->hr);
    norm_input_vector(data->acc_x);
    norm_input_vector(data->acc_y);
    norm_input_vector(data->acc_z);
    norm_input_vector(data->hr);
}

inline void Preprocessor::find_min_max(const std::unique_ptr<input_vector> &input) const {
    double min_value = std::numeric_limits<double>::max();
    double max_value = -std::numeric_limits<double>::max();

    auto arr = input->values;

    for(double& value : arr){
        min_value = std::min(min_value, value);
        max_value = std::max(max_value, value);
    }
    input->min = min_value;
    input->max = max_value;
}

bool Preprocessor::analyze_files(const std::string &hr_file, const std::string &acc_file,
                                 const std::shared_ptr<input_data> &result) {

    std::ifstream hfile(hr_file);
    if (hfile.is_open()) {
        std::string line;
        result->first_hr_time = get_first_entry_date(hfile, line, result->hr_date_end_index);
        hfile.close();
    }else{
        std::cerr << "File cannot be opened " << hr_file << std::endl;
        return false;
    }
    std::ifstream afile(acc_file);
    if (afile.is_open()) {
        std::string line;

        //we need to get sampling rate to possibly pair with heart rate
        uint8_t sampling_rate = 1;
        result->first_acc_time = get_first_entry_date(afile, line, result->acc_date_end_index);

        while(std::getline(afile, line)){
            std::string date = line.substr(0, result->acc_date_end_index);
            auto time = parse_date(date);
            if(time > result->first_acc_time){
                break;
            }
            ++sampling_rate;
        }
        result->sampling_rate = sampling_rate;
        afile.close();
    }else{
        std::cerr << "File cannot be opened " << hr_file << std::endl;
        return false;
    }
    return true;
}

time_t Preprocessor::get_first_entry_date(std::ifstream &file, std::string &line, size_t& date_end_index) {
    std::getline(file, line); //skip first line (header)
    if(std::getline(file, line)){
        date_end_index = line.find(',');
        std::string date = line.substr(0, date_end_index);
        time_t entry_time = parse_date(date);
        return entry_time;
    }
    return 0;
}



void Preprocessor::calculate_hr_init_data(const std::shared_ptr<input_data>& input) {

    double sum = 0;
    double sum_power_2 = 0;

    for (const auto hr: input->hr->values) {
        sum += hr;
        sum_power_2 += (hr * hr);
    }

    input->hr_sum = sum;
    input->squared_hr_corr_sum = (input->hr_entries_count * sum_power_2 - (sum * sum));
}

Preprocessor::Preprocessor(std::string  input_folder) : input_folder(std::move(input_folder)) {
}


//
// Created by pulta on 26.11.2023.
//

#include "ParallelPreprocessor.h"
#include "../utils.h"
#include <fstream>
#include <ppl.h>
#include <iostream>
#include <execution>

void ParallelPreprocessor::load_hr_file_content(const std::shared_ptr<input_data> &result, std::ifstream &file) const {
    Preprocessor::load_hr_file_content(result, file);
}

void ParallelPreprocessor::load_acc_file_content(const std::shared_ptr<input_data> &result, std::ifstream &file) const {
    std::string line;
    std::getline(file, line); //skip first line (header)

    //first filter all lines that are before our wanted time
    skip_past_time_entries(result->first_hr_time, result->acc_date_end_index, file, line);

    const uint8_t sampling_rate = result->sampling_rate;

    const size_t offset = result->acc_entries_count;
    const size_t entries_count = result->hr_entries_count - offset;
    std::vector<acc_minute_sample> all_lines (entries_count,
                                              acc_minute_sample(sampling_rate));
    uint8_t sample_count = 0;
    size_t entry_count = 0;
    //first load all lines into buffers
    do{
        all_lines[entry_count].lines[sample_count] = line;
        ++sample_count;
        if(sample_count >= sampling_rate){
            all_lines[entry_count].index = entry_count;
            sample_count = 0;
            ++entry_count;
            if(entry_count >= entries_count){
                break;
            }
        }
    }while(std::getline(file, line));

    auto& x_input = result->acc_x->values;
    auto& y_input = result->acc_y->values;
    auto& z_input = result->acc_z->values;
    //now we can start saving some data
    const size_t data_start_index = result->acc_date_end_index + 1;
    std::for_each(std::execution::par,begin(all_lines), end(all_lines),
                  [&](const acc_minute_sample& sample){

        double x_result = 0;
        double y_result = 0;
        double z_result = 0;
        for (const auto& line: sample.lines) {
            auto x_value_end = line.find(',', data_start_index);
            auto y_value_end = line.find(',', x_value_end + 1);

            auto x = line.substr(data_start_index, x_value_end - data_start_index);
            auto y = line.substr(x_value_end + 1, y_value_end - x_value_end - 1);
            auto z = line.substr(y_value_end + 1, line.size() - y_value_end - 1);

            x_result += std::stod(x);
            y_result += std::stod(y);
            z_result += std::stod(z);
        }
        x_input[offset + sample.index] = x_result;
        y_input[offset + sample.index] = y_result;
        z_input[offset + sample.index] = z_result;
    });

    result->acc_entries_count += entry_count - entry_count % VECTOR_SIZE;
    std::cout << "Loaded " << entry_count << " entries from acc file" << std::endl;
}

void ParallelPreprocessor::norm_input_vector(const std::unique_ptr<input_vector> &input) const {
    auto max = input->max;
    auto min = input->min;
    auto& arr = input->values;
    const size_t count = arr.size();
    const auto scope = (max - min);
    #pragma loop(hint_parallel(VECTOR_SIZE_MACRO))
    for(size_t i = 0; i < count; ++i){
        auto value = arr[i];
        arr[i] = (value - min) / scope;
    }
}

void ParallelPreprocessor::find_min_max(const std::unique_ptr<input_vector> &input) const {
    const auto values = input->values;
    const auto [min, max]
                                    = std::minmax_element(values.begin(), values.end());

    input->min = *min;
    input->max = *max;
}

void ParallelPreprocessor::preprocess_acc_vectors(const std::shared_ptr<input_data> &result,
                                                  size_t current_offset) const {
    auto& x_input = result->acc_x->values;
    auto& y_input = result->acc_y->values;
    auto& z_input = result->acc_z->values;
    const uint8_t sampling_rate = result->sampling_rate;
    //are there are more entries in the hr vector than the acc vector?
    if(result->acc_entries_count < result->hr_entries_count){
        result->hr_entries_count = result->acc_entries_count;
    }

    const auto count = result->acc_entries_count;
#pragma loop(hint_parallel(VECTOR_SIZE_MACRO))
    for(size_t i = current_offset; i < count; ++i){
        x_input[i] /= sampling_rate;
        y_input[i] /= sampling_rate;
        z_input[i] /= sampling_rate;
    }
}

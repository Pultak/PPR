
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
//    Preprocessor::load_hr_file_content(result, file);
//    return;
    std::string line;

    skip_past_time_entries(result->first_acc_time, result->hr_date_end_index, file, line);

    std::vector<std::string> all_lines;
    all_lines.reserve(result->hr_entries_count);
    size_t count = 0;
    do {
        all_lines.emplace_back(line);
        ++count;
    } while (std::getline(file, line));

    result->hr = std::make_unique<input_vector>(count);
    auto& hr_input = result->hr->values;
    const size_t data_start_index = result->hr_date_end_index + 1;

    std::for_each(std::execution::par,begin(all_lines), end(all_lines), [&](const std::string &line) {
        auto hr = line.substr(data_start_index, 6); //6 chars is the maximum possible size of hr entry
        hr_input[0] = std::stod(hr);
    });
    result->hr_entries_count = count - count % VECTOR_SIZE;
    hr_input.resize(result->hr_entries_count);
}

void ParallelPreprocessor::load_acc_file_content(const std::shared_ptr<input_data> &result, std::ifstream &file) const {
//    Preprocessor::load_acc_file_content(result, file);
//    return;
    std::string line;
    std::getline(file, line); //skip first line (header)

    //first filter all lines that are before our wanted time
    skip_past_time_entries(result->first_hr_time, result->acc_date_end_index, file, line);

    const uint8_t sampling_rate = result->sampling_rate;

    std::vector<acc_minute_sample> all_lines (result->hr_entries_count,
                                              acc_minute_sample(sampling_rate));
    uint8_t sample_count = 0;
    size_t entry_count = 0;
    const size_t entries_count = result->hr_entries_count;
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

    result->acc_x = std::make_unique<input_vector>(entry_count);
    result->acc_y = std::make_unique<input_vector>(entry_count);
    result->acc_z = std::make_unique<input_vector>(entry_count);
    auto& x_input = result->acc_x->values;
    auto& y_input = result->acc_y->values;
    auto& z_input = result->acc_z->values;
    //now we can start saving some data
    const size_t data_start_index = result->acc_date_end_index + 1;
    std::for_each(std::execution::par,begin(all_lines), end(all_lines), [&](const acc_minute_sample& sample){

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
        x_input[sample.index] = x_result;
        y_input[sample.index] = y_result;
        z_input[sample.index] = z_result;
    });

    result->acc_entries_count = entry_count - entry_count % VECTOR_SIZE;
}

void ParallelPreprocessor::norm_input_vector(const std::unique_ptr<input_vector> &input) const {
    auto max = input->max;
    auto min = input->min;
    std::for_each(std::execution::par,begin(input->values), end(input->values), [&](double& value){
        value = (value - min) / (max - min);
    });
}

void ParallelPreprocessor::find_min_max(const std::unique_ptr<input_vector> &input) const {
    double min_value = std::numeric_limits<double>::max();
    double max_value = std::numeric_limits<double>::min();
    std::for_each(std::execution::par,begin(input->values), end(input->values), [&](double value){
        min_value = std::min(min_value, value);
        max_value = std::max(max_value, value);
    });
    input->min = min_value;
    input->max = max_value;
}

void ParallelPreprocessor::preprocess_vectors(const std::shared_ptr<input_data> &result) const {
    auto& x_input = result->acc_x->values;
    auto& y_input = result->acc_y->values;
    auto& z_input = result->acc_z->values;
    const uint8_t sampling_rate = result->sampling_rate;
    //are there are more entries in the hr vector than the acc vector?
    if(result->acc_entries_count < result->hr_entries_count){
        x_input.resize(result->acc_entries_count);
        y_input.resize(result->acc_entries_count);
        z_input.resize(result->acc_entries_count);

        result->hr_entries_count = result->acc_entries_count;
    }
    result->hr->values.resize(result->acc_entries_count);

    std::for_each(std::execution::par, begin(x_input), end(x_input), [&](double& value){
        value /= sampling_rate;
    });
    std::for_each(std::execution::par,begin(y_input), end(y_input), [&](double& value){
        value /= sampling_rate;
    });
    std::for_each(std::execution::par,begin(z_input), end(z_input), [&](double& value){
        value /= sampling_rate;
    });
}

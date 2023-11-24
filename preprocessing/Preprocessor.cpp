//
// Created by pulta on 23.10.2023.
//

#include <fstream>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <ppl.h>
#include <limits>
#include <filesystem>
#include <stack>
#include "Preprocessor.h"
#include "../utils.h"


bool Preprocessor::load_and_preprocess(std::string& hr_file, std::string& acc_file, const std::shared_ptr<input_data>& result) {

    //todo make sure to cut the size
    auto vector_size = get_data_vector_needed_size(hr_file);

    bool read_ok = read_file_content( hr_file, 0, result, vector_size) &&
            read_file_content( acc_file, result->first_hr_time, result, vector_size);
    if(!read_ok){
        std::cerr << "Input load failed!" << std::endl;
        return false;
    }
    //filter_input_data(result); todo we dont need this when using fixed size
    //todo find min + max
    __int64 elapsed = time_call([&] {
        normalize_data(result);
    });
    std::cout << "Normalization of input data took " << elapsed << " ms" << std::endl;
    return true;
}

bool Preprocessor::load_and_preprocess_folder(const std::string& input_folder, const std::shared_ptr<input_data> &result) {
    directory_map directories {};
    collect_directory_data_files_entries(input_folder, directories);
    std::cout << "Loaded total of " << directories.size() << " directories!" << std::endl;
    //todo
    for (const auto& data_entry: directories) {
        //todo local result and then merge
        std::string hr_file = data_entry.second.first;
        std::string acc_file = data_entry.second.second;
        if(!load_and_preprocess(hr_file, acc_file, result)){
            return false;
        }
    }
    return true;
}


file_type is_data_file(const std::string& file_name){
    if(file_name.compare(0, 3, "HR_") == 0){
        return file_type::HR_DATA_FILE;
    }else if(file_name.compare(0, 4, "ACC_") == 0){
        return file_type::ACC_DATA_FILE;
    }
    return file_type::NOT_DATA_FILE;
}

void Preprocessor::collect_directory_data_files_entries(const std::filesystem::path& root_path, directory_map& folder_map) {
    std::stack<std::filesystem::path> pathStack;
    pathStack.push(root_path);

    while (!pathStack.empty()) {
        std::filesystem::path currentPath = pathStack.top();
        pathStack.pop();

        for (const auto& entry: std::filesystem::directory_iterator(currentPath)) {
            if (entry.is_directory()) {
                std::cout << "Folder: " << entry.path() << "\n"; //todo comment
                pathStack.push(entry.path());
            } else if (entry.is_regular_file()) {
                std::cout << "File: " << entry.path() << "\n";
                const auto& file_path = entry.path();
                auto parent_folder_name = file_path.parent_path().filename().string();
                auto file_type = is_data_file(file_path.filename().string());
                if(file_type == file_type::NOT_DATA_FILE){
                    continue;
                }
                //todo do I need to check if file has good suffix (.._folderName)
//                    if(file_path.filename().string() == ""){
//
//                    }
                if(folder_map.find(parent_folder_name) == folder_map.end()){
                    folder_map[parent_folder_name] = std::make_pair("", "");
                }
                switch (file_type) {
                    case HR_DATA_FILE:
                        folder_map[parent_folder_name].first = file_path.string();
                        break;
                    case ACC_DATA_FILE:
                        folder_map[parent_folder_name].second = file_path.string();
                        break;
                    default:
                        std::cerr << "Unknown error occurred during directory traversing." << std::endl;
                }
            }
        }
    }
}

size_t Preprocessor::get_data_vector_needed_size(std::string& input_file){

    auto file_size = std::filesystem::file_size(input_file); //todo when doing iterating over folder get full file_size

    const size_t line_char_count = 24; //there are 24-25 characters per line typically //todo constant

    size_t vector_size = file_size / line_char_count;
    //cut it so it can be easily vectorized
    vector_size -= vector_size % VECTOR_SIZE;

    return vector_size;
}

bool Preprocessor::read_file_content(std::string& input_file, time_t hr_begin_time,
                                     const std::shared_ptr<input_data> &result, size_t vector_size) {
    std::ifstream file(input_file);
    if (file.is_open()) {

        __int64 elapsed = time_call([&] {
            if (hr_begin_time) {
                //hr_begin_time is not 0, so we are loading acc file
                result->acc_x = std::make_unique<input_vector>(vector_size);
                result->acc_y = std::make_unique<input_vector>(vector_size);
                result->acc_z = std::make_unique<input_vector>(vector_size);
                load_acc_file_content(hr_begin_time, result, file);
            } else {
                result->hr = std::make_unique<input_vector>(vector_size);
                load_hr_file_content(result, file);
            }
            file.close();
        });
        std::cout << "Parsing of file (" << input_file << ") took " << elapsed << " ms" << std::endl;
        return true;
    } else {
        std::cerr << "Error: Unable to open the file." << std::endl;
        //todo exit app?
        return false;
    }
}

void Preprocessor::load_hr_file_content(const std::shared_ptr<input_data> &result, std::ifstream &file) const {
    auto& hr_input = result->hr->values;
    std::string line;

//    std::stringstream buffer;
//    buffer << file.rdbuf();

    //todo get position of first comma

    std::getline(file, line); //skip first line (header)
    if(std::getline(file, line)){
        tm tm = {};
        std::string date;
        std::istringstream iss(line);
        std::getline(iss, date, ',') >> std::get_time(&tm, "%b %d %Y %H:%M:%S");

        std::stringstream ss(date);
        ss >> std::get_time(&tm, time_format);
        time_t entry_time = mktime(&tm);
        result->first_hr_time = entry_time;
    }

    size_t count = 0;
    do {
        std::istringstream iss(line);
//        auto date = line.substr(0, 19); //todo
        auto hr = line.substr(20, 6);//todo number scary
        hr_input[count] = std::stod(hr);
        ++count;

    } while (std::getline(file, line));
    result->entries_count = count;
}

void Preprocessor::load_acc_file_content(time_t hr_begin_time, const std::shared_ptr<input_data> &result,
                                         std::ifstream &file) const {
    auto& x_input = result->acc_x->values;
    auto& y_input = result->acc_y->values;
    auto& z_input = result->acc_z->values;
    time_t previous_time;
    std::string line;

    std::stringstream buffer;
    buffer << file.rdbuf();
    //todo get only lines equal to size of the hr file
    //first filter all lines that are before our wanted time
    skip_past_entries(hr_begin_time, buffer, line, previous_time);

    //now we can start saving some data
//    uint8_t sampling_rate = get_sampling_rate();
    uint8_t sampling_rate = 32; //todo
    tm tm = {};
    uint8_t count = 0;
    size_t index = 0;
    do{//todo vectorize this shhh which could be done by having multiple methods with specific vector size for vectorization

//        auto date = line.substr(0, 26);
        auto x_value_end = line.find(',', 27); //todo scary number
        auto y_value_end = line.find(',', x_value_end + 1);

        auto x = line.substr(27, x_value_end - 27);
        auto y = line.substr(x_value_end + 1, y_value_end - x_value_end - 1);
        auto z = line.substr(y_value_end + 1, line.size() - y_value_end - 1);

        x_input[index] += std::stod(x);
        y_input[index] += std::stod(y);
        z_input[index] += std::stod(z);
        ++count;
        if(count >= 32){
            count = 0;
            ++index;
        }

    }while (std::getline(buffer, line));

//    myVector.shrink_to_fit(); //todo use this?

    concurrency::parallel_for_each(begin(x_input), begin(x_input) + result->entries_count, [&](double& value){
        value /= sampling_rate;
    });
    //todo
    concurrency::parallel_for_each(begin(y_input), end(y_input), [&](double& value){
        value /= sampling_rate;
    });
    //todo
    concurrency::parallel_for_each(begin(z_input), end(z_input), [&](double& value){
        value /= sampling_rate;
    });

}

void Preprocessor::skip_past_entries(time_t hr_begin_time, std::stringstream &file, std::string &result_line,
                                     time_t &previous_time) const {
    std::tm tm = {};
    std::string line;
    while (std::getline(file, line)) { //todo do the skip before loading the whole file
        std::string date;
        std::istringstream iss(line);
        std::getline(iss, date, ',') >> std::get_time(&tm, "%b %d %Y %H:%M:%S");

        std::stringstream ss(date);
        ss >> std::get_time(&tm, time_format);
        time_t time = mktime(&tm);
        if(time < hr_begin_time){
            continue;
        }else{
            previous_time = time;
            result_line = line;
            break;
        }
    }
}

void Preprocessor::filter_input_data(const std::shared_ptr<input_data> &inputData) {
    //difftime returns seconds
    double diff_minutes = difftime(inputData->first_hr_time, inputData->first_acc_time) / 60;
    auto del_entries_count = (int) abs(diff_minutes);

    std::vector<double> &hr_input = inputData->hr->values;
    if(del_entries_count > 0){
        //remove hr entries that are before acc values
        hr_input.erase(hr_input.begin(), hr_input.begin() + del_entries_count);
        inputData->entries_count -= del_entries_count;
    }

    uint8_t size_remainder = inputData->entries_count % VECTOR_SIZE;
    if(size_remainder != 0){
        std::vector<double> &acc_x = inputData->acc_x->values;
        std::vector<double> &acc_y = inputData->acc_y->values;
        std::vector<double> &acc_z = inputData->acc_z->values;

        hr_input.erase(hr_input.end() - size_remainder, hr_input.end());
        acc_x.erase(acc_x.end() - size_remainder, acc_x.end());
        acc_y.erase(acc_y.end() - size_remainder, acc_y.end());
        acc_z.erase(acc_z.end() - size_remainder, acc_z.end());

        inputData->entries_count -= size_remainder;
    }
}

void norm_input_vector(const std::unique_ptr<input_vector>& vector){
    auto max = vector->max;
    auto min = vector->min;
    concurrency::parallel_for_each(begin(vector->values), end(vector->values), [&](double& value){
        value = (value - min) / (max - min);
    });
}

void Preprocessor::normalize_data(const std::shared_ptr<input_data> &data) {
    find_min_max(data->acc_x);
    find_min_max(data->acc_y);
    find_min_max(data->acc_z);
    find_min_max(data->hr);
    norm_input_vector(data->acc_x);
    norm_input_vector(data->acc_y);
    norm_input_vector(data->acc_z);
    norm_input_vector(data->hr);
}

void Preprocessor::find_min_max(const std::unique_ptr<input_vector> &input) {
    double min_value = std::numeric_limits<double>::max();
    double max_value = std::numeric_limits<double>::min();
    concurrency::parallel_for_each(begin(input->values), end(input->values), [&](double value){
        min_value = std::min(min_value, value);
        max_value = std::max(max_value, value);
    });
    input->min = min_value;
    input->max = max_value;
}

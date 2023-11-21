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
#include "../consts.h"


bool Preprocessor::load_and_preprocess(std::string& hr_file, std::string& acc_file, const std::shared_ptr<input_data>& result) {

    bool read_ok = read_file_content( hr_file, 0, result) &&
            read_file_content( acc_file, result->first_hr_time, result);
    if(!read_ok){
        std::cerr << "Input load failed!" << std::endl;
        return false;
    }
    filter_input_data(result);
    normalize_data(result);
    return true;
}

bool Preprocessor::load_and_preprocess_folder(const std::string& input_folder, const std::shared_ptr<input_data> &result) {
    directory_map directories {};
    collect_directory_data_files_entries(input_folder, directories);
    std::cout << "Loaded total of " << directories.size() << " directories!" << std::endl;
    //todo
    for (const auto& data_entry: directories) {
        //todo local result and then merge
        if(!load_and_preprocess(data_entry.first, data_entry.second, result)){
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

//        try {
        for (const auto& entry: std::filesystem::directory_iterator(currentPath)) {
            if (entry.is_directory()) {
                std::cout << "Folder: " << entry.path() << "\n"; //todo comment
                pathStack.push(entry.path());
            } else if (entry.is_regular_file()) {
                std::cout << "File: " << entry.path() << "\n";
                const auto& file_path = entry.path();
                auto parent_folder_name = file_path.parent_path().filename().string();
                auto file_type = is_data_file(file_path.string());
                if(file_type == file_type::NOT_DATA_FILE){
                    continue;
                }
                //todo do I need to check if file has good suffix (.._folderName)
//                    if(file_path.filename().string() == ""){
//
//                    }
                if(folder_map.find(parent_folder_name) != folder_map.end()){
                    folder_map[parent_folder_name] = std::make_pair("", "");
                }
                switch (file_type) {
                    case HR_DATA_FILE:
                        folder_map[parent_folder_name].first = file_path.string();
                        break;
                    case ACC_DATA_FILE:
                        folder_map[parent_folder_name].second = file_path.string();
                        break;
                    case NOT_DATA_FILE:
                    default:
                        std::cerr << "Unknown error occurred during directory traversing." << std::endl;
                        break;
                }
            }
        }
//        } catch (const std::exception& e) {
//            std::cerr << "Error: " << e.what() << "\n"; //todo better exception handling
//        }
    }
}


bool Preprocessor::read_file_content(const std::string& input_file, time_t hr_begin_time, const std::shared_ptr<input_data> &result) {
    std::ifstream file(input_file);

    if (file.is_open()) {
        if(hr_begin_time){
            //hr_begin_time is not 0, so we are loading acc file
            load_acc_file_content(hr_begin_time, result, file);
        }else{
            load_hr_file_content(result, file);
        }
        file.close();
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
        std::string date;
        double hr;

        //todo time value can be skipped
        if (std::getline(iss, date, ',') && (iss >> hr)) {
            hr_input.push_back(hr);
            ++count;
        }
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
    //first filter all lines that are before our wanted time
    skip_past_entries(hr_begin_time, file, line, previous_time);

    //now we can start saving some data
    int count = 0;
    tm tm = {};
    double val_x_sum, val_y_sum, val_z_sum;
    do{
        std::istringstream iss(line);
        std::string date;
        double val_x, val_y, val_z;

        if (std::getline(iss, date, ',') &&
            (iss >> val_x) &&
            (iss.ignore(1, ',') >> val_y) &&
            (iss.ignore(1, ',') >> val_z)) {

            std::stringstream ss(date);
            ss >> std::get_time(&tm, time_format);
            time_t entry_time = mktime(&tm);

            if(entry_time == previous_time){
                val_x_sum += val_x;
                val_y_sum += val_y;
                val_z_sum += val_z;
                ++count;
            }else{
                x_input.push_back(val_x_sum / count);
                y_input.push_back(val_y_sum / count);
                z_input.push_back(val_z_sum / count);
                previous_time = entry_time;
                val_x_sum = val_x;
                val_y_sum = val_y;
                val_z_sum = val_z;
                count = 0;
            }
        }
    }while (std::getline(file, line));
}

void Preprocessor::skip_past_entries(time_t hr_begin_time, std::ifstream &file, std::string &result_line,
                                     time_t &previous_time) const {
    std::tm tm = {};
    std::string line;
    while (std::getline(file, line)) {
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

void Preprocessor::normalize_data(const std::shared_ptr<input_data> &data) {
    const auto& input = data->hr;
    find_min_max(data->acc_x);
    find_min_max(data->acc_y);
    find_min_max(data->acc_z);

    find_min_max(input);
    auto max = input->max;
    auto min = input->min;
    //todo maybe vectorization better
    concurrency::parallel_for_each(begin(input->values), end(input->values), [&](double& value){
        value = (value - min) / (max - min);
    });
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

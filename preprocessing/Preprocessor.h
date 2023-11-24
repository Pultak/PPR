//
// Created by pulta on 23.10.2023.
//

#ifndef OCL_TEST_PREPROCESSOR_H
#define OCL_TEST_PREPROCESSOR_H


#include <vector>
#include <string>
#include <memory>
#include <map>
#include <filesystem>
#include <random>


typedef std::map<std::string, std::pair<std::string, std::string>> directory_map;

struct input_vector{

    std::vector<double> values;
    double min = DBL_MIN;
    double max = DBL_MAX;

    explicit input_vector(uintmax_t value_count): values(value_count){
    }
};

struct input_data{
    std::unique_ptr<input_vector> acc_x = nullptr;
    std::unique_ptr<input_vector> acc_y = nullptr;
    std::unique_ptr<input_vector> acc_z = nullptr;
    std::unique_ptr<input_vector> hr = nullptr;
    size_t entries_count = 0;
    time_t first_acc_time = 0;
    time_t first_hr_time = 0;
};

enum file_type{
    NOT_DATA_FILE, HR_DATA_FILE, ACC_DATA_FILE
};

class Preprocessor {

private:
    const char* time_format = "%Y-%m-%d %H:%M:%S";


public:
    bool load_and_preprocess(std::string& hr_file, std::string& acc_file, const std::shared_ptr<input_data>& result);
    bool load_and_preprocess_folder(const std::string& input_folder, const std::shared_ptr<input_data> &result);


private:
    bool read_file_content(std::string& input_file, time_t hr_begin_time, const std::shared_ptr<input_data> &result, size_t vector_size);

    void filter_input_data(const std::shared_ptr<input_data> &inputData);

    static void normalize_data(const std::shared_ptr<input_data> &data);

    void skip_past_entries(time_t hr_begin_time, std::stringstream &file, std::string &result_line, time_t &previous_time) const;

    void load_acc_file_content(time_t hr_begin_time, const std::shared_ptr<input_data> &result, std::ifstream &file) const;

    void load_hr_file_content(const std::shared_ptr<input_data> &result, std::ifstream &file) const;

    static void find_min_max(const std::unique_ptr<input_vector> &input);

    static void collect_directory_data_files_entries(const std::filesystem::path& root_path, directory_map& folder_map);

    static size_t get_data_vector_needed_size(std::string& input_file);
};


#endif //OCL_TEST_PREPROCESSOR_H

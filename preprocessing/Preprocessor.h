//
// Created by pulta on 23.10.2023.
//

#ifndef OCL_TEST_PREPROCESSOR_H
#define OCL_TEST_PREPROCESSOR_H


#include <vector>
#include <string>


struct input_vector{
    std::vector<double> values;
    double min;
    double max;
};

struct input_data{
    std::unique_ptr<input_vector> acc_x = std::make_unique<input_vector>();
    std::unique_ptr<input_vector> acc_y = std::make_unique<input_vector>();
    std::unique_ptr<input_vector> acc_z = std::make_unique<input_vector>();
    std::unique_ptr<input_vector> hr = std::make_unique<input_vector>();
    size_t entries_count = 0;
    time_t first_acc_time = 0;
    time_t first_hr_time = 0;
};


class Preprocessor {

private:
    const char* time_format = "%Y-%m-%d %H:%M:%S";


public:
    bool load_and_preprocess(const std::string& input_folder, std::unique_ptr<input_data> &result);
    void load_and_preprocess_folder(char input_folder[]);


private:
    bool read_file_content(const std::string& input_file, time_t hr_begin_time, std::unique_ptr<input_data> &result);

    void filter_input_data(std::unique_ptr<input_data> &inputData);

    static void normalize_data(std::unique_ptr<input_data> &data);

    void skip_past_entries(time_t hr_begin_time, std::ifstream &file, std::string &result_line, time_t &previous_time) const;

    void load_acc_file_content(time_t hr_begin_time, std::unique_ptr<input_data> &result, std::ifstream &file) const;

    void load_hr_file_content(std::unique_ptr<input_data> &result, std::ifstream &file) const;

    static void find_min_max(std::unique_ptr<input_vector>& input) ;
};


#endif //OCL_TEST_PREPROCESSOR_H

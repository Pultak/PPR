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
    explicit input_vector() = default;
};

struct input_data{
    std::unique_ptr<input_vector> acc_x = nullptr;
    std::unique_ptr<input_vector> acc_y = nullptr;
    std::unique_ptr<input_vector> acc_z = nullptr;
    std::unique_ptr<input_vector> hr = nullptr;

    size_t hr_entries_count = 0;
    size_t acc_entries_count = 0;

    time_t first_hr_time = 0;
    time_t first_acc_time = 0;
    uint8_t sampling_rate = 0;
    size_t hr_date_end_index = 0;
    size_t acc_date_end_index = 0;

    double hr_sum = 0;
    double squared_hr_corr_sum = 0;
};

enum file_type{
    NOT_DATA_FILE, HR_DATA_FILE, ACC_DATA_FILE
};

class Preprocessor {

public:
    Preprocessor(std::string  input_folder);

public:
    bool load_and_preprocess(std::string &hr_file, std::string &acc_file,
                             const std::shared_ptr<input_data> &result, const std::string &folder_name) const;
    bool load_and_preprocess_folder(const std::shared_ptr<input_data> &result);

    virtual void find_min_max(const std::unique_ptr<input_vector> &input) const;

protected:
    const std::string input_folder;
    uintmax_t total_predicted_input_size = 0;

protected:
    void process_file_content(const std::string &input_file, bool is_acc_file,
                                            const std::shared_ptr<input_data> &result) const;

    virtual void load_acc_file_content(const std::shared_ptr<input_data> &result, std::ifstream &file) const;

    virtual void load_hr_file_content(const std::shared_ptr<input_data> &result, std::ifstream &file) const;

    virtual void preprocess_vectors(const std::shared_ptr<input_data> &result, size_t current_offset) const;

    void normalize_data(const std::shared_ptr<input_data> &data) const;


    virtual void norm_input_vector(const std::unique_ptr<input_vector> &input) const;

    void collect_directory_data_files_entries(directory_map& folder_map);

protected:

    static void skip_past_time_entries(time_t begin_time, size_t date_end_index, std::ifstream &file,
                                       std::string &result_line) ;

    static bool analyze_files(const std::string &hr_file, const std::string &acc_file, const std::shared_ptr<input_data> &result);

    static size_t get_data_vector_needed_size(const std::string& input_file);

    static time_t get_first_entry_date(std::ifstream &file, std::string &line, size_t& date_end_index);
    static void calculate_hr_init_data(const std::shared_ptr<input_data>& input);

    static void print_input_data_statistics(const std::shared_ptr<input_data> &result) ;
};


#endif //OCL_TEST_PREPROCESSOR_H

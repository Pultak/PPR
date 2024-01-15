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

/// definition of type holding all input files
typedef std::map<std::string, std::pair<std::string, std::string>> directory_map;

struct input_vector{
    std::vector<double> values;
    double min = DBL_MIN;
    double max = DBL_MAX;

    explicit input_vector(uintmax_t value_count): values(value_count){
    }
    explicit input_vector() = default;
};

/// Object to hold input data and all needed variables
struct input_data{
    std::unique_ptr<input_vector> acc_x = nullptr;
    std::unique_ptr<input_vector> acc_y = nullptr;
    std::unique_ptr<input_vector> acc_z = nullptr;
    std::unique_ptr<input_vector> hr = nullptr;

    /// count of hr entries needed for calculating offset etc
    size_t hr_entries_count = 0;
    size_t acc_entries_count = 0;

    /// date time of the first entry in current hr file
    time_t first_hr_time = 0;
    /// date time of the first entry in current acc file
    time_t first_acc_time = 0;
    /// sampling rate of the current acc file
    uint8_t sampling_rate = 0;
    /// index of the date time separator
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
    explicit Preprocessor(std::string  input_folder);

public:
    /// Loads and preprocess the pair of ACC and HR file and saves the data into result vectors
    /// \param hr_file path to the hr_file
    /// \param acc_file path to the acc_file
    /// \param result object holding all result vectors
    /// \param folder_name name of the folder that is being processed
    /// \return true if the processing went ok
    bool load_and_preprocess(std::string &hr_file, std::string &acc_file,
                             const std::shared_ptr<input_data> &result, const std::string &folder_name) const;

    /// Goes though folder files and processes all hr and acc pairs
    /// \param result empty object holding the preprocessing results
    /// \return true if everything went well
    bool load_and_preprocess_folder(const std::shared_ptr<input_data> &result);

    /// Method finds min and max values from the passed input vector
    /// \param input input vector
    virtual void find_min_max(const std::unique_ptr<input_vector> &input) const;

protected:
    const std::string input_folder;
    uintmax_t total_predicted_input_size = 0;

protected:
    /// General method for file parsing measuring the time needed for processing, opening file
    /// \param input_file path to the file
    /// \param is_acc_file flag indicating that the file has acc data
    /// \param result object holding the result vectors
    void process_file_content(const std::string &input_file, bool is_acc_file,
                                            const std::shared_ptr<input_data> &result) const;

    /// Method for parsing of the accelerator data file
    /// \param result object holding the result vectors
    /// \param file opened file input stream
    virtual void load_acc_file_content(const std::shared_ptr<input_data> &result, std::ifstream &file) const;

    /// Method for processing of the heart rate data file
    /// \param result object holding the result vectors
    /// \param file opened file input stream
    virtual void load_hr_file_content(const std::shared_ptr<input_data> &result, std::ifstream &file) const;

    /// Method to finalize vector sizes and use sampling rate to mean the acc data values
    /// \param result object holding last loaded data with file specific sampling rate
    /// \param current_offset offset on the global input data vector
    virtual void preprocess_acc_vectors(const std::shared_ptr<input_data> &result, size_t current_offset) const;

    /// Method does all processing needed after the whole database load
    /// \param data data that needs to be postprocessed
    void post_process(const std::shared_ptr<input_data> &data) const;


    /// min-max normalizes the input vector
    /// \param input input vector
    virtual void norm_input_vector(const std::unique_ptr<input_vector> &input) const;

    /// Method to collect all hr and acc file pairs from the passed directory
    /// \param folder_map result folder map with pairs
    void collect_directory_data_files_entries(directory_map& folder_map);

protected:

    /// Method used to equalize the time differences at the start of the hr and acc pair files
    /// \param begin_time start date time of the opposing file from pair
    /// \param date_end_index index of separator in line string
    /// \param file opened file input stream
    /// \param result_line last line that was checked
    static void skip_past_time_entries(time_t begin_time, size_t date_end_index, std::ifstream &file,
                                       std::string &result_line);

    /// Gets all needed file variables from the acc and hr file pair
    /// \param hr_file path to hr file
    /// \param acc_file path to acc file
    /// \param result object holding the variables
    /// \return true if the file is ok
    static bool analyze_files(const std::string &hr_file, const std::string &acc_file,
                              const std::shared_ptr<input_data> &result);

    /// Predicts the size needed for vector initialization for passed file
    /// \param input_file file that size should be predicted
    /// \return vector size needed
    static size_t get_data_vector_needed_size(const std::string& input_file);

    /// Gets the date time from the first line of the file
    /// \param file opened input file stream
    /// \param line last loaded line
    /// \param date_end_index index of separator for the date time
    /// \return parsed date time
    static time_t get_first_entry_date(std::ifstream &file, std::string &line, size_t& date_end_index);

    /// Calculates sum over the hr data needed later for correlation
    /// \param input object holding all loaded data
    static void calculate_hr_init_data(const std::shared_ptr<input_data>& input);

    static void print_input_data_statistics(const std::shared_ptr<input_data> &result) ;
};


#endif //OCL_TEST_PREPROCESSOR_H

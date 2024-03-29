//
// Created by pulta on 26.11.2023.
//

#ifndef OCL_TEST_PARALLELPREPROCESSOR_H
#define OCL_TEST_PARALLELPREPROCESSOR_H


#include "Preprocessor.h"

#include <utility>

struct acc_minute_sample{
    size_t index{};
    std::vector<std::string> lines;
public:
    explicit acc_minute_sample(const uint8_t sampling_size) : lines(sampling_size){
    }
};


class ParallelPreprocessor : public Preprocessor {

public:
    explicit ParallelPreprocessor(const std::string& input_folder): Preprocessor(input_folder){};

    void find_min_max(const std::unique_ptr<input_vector> &input) const override;

protected:

    void load_acc_file_content(const std::shared_ptr<input_data> &result, std::ifstream &file) const override;

    void preprocess_acc_vectors(const std::shared_ptr<input_data> &result, size_t current_offset) const override;

    void load_hr_file_content(const std::shared_ptr<input_data> &result, std::ifstream &file) const override;

    void norm_input_vector(const std::unique_ptr<input_vector> &input) const override;
};


#endif //OCL_TEST_PARALLELPREPROCESSOR_H

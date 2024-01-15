#include <iostream>
#include <fstream>
#include "preprocessing/Preprocessor.h"
#include "computation/CalculationScheduler.h"
#include "preprocessing/ParallelPreprocessor.h"

#include <ppl.h>
#include <array>

#include "computation/gpu/OpenCLComponent.h"
#include "computation/ParallelCalculationScheduler.h"


void init_svg_header(std::ofstream &svgFile, double hr_min, double hr_max, double min_acc,
                     double max_acc);

int createSVG(std::unique_ptr<input_vector>& hr, std::unique_ptr<input_vector>& trs_acc){
    // Open an output file for writing
    std::ofstream svgFile("graph.svg");

    if (!svgFile.is_open()) {
        std::cerr << "Failed to open SVG file." << std::endl;
        return 1;
    }
    std::cout << "Generating SVG graph." << std::endl;

    const double hr_min = hr->min;
    const double hr_max = hr->max;
    const double min_acc = trs_acc->min;
    const double max_acc = trs_acc->max;
    init_svg_header(svgFile, hr_min, hr_max, min_acc, max_acc);

    svgFile << "<!-- data -->\n";

    const size_t count = hr->values.size();
    const auto scope = (max_acc - min_acc);

    std::map<double, std::map<double, double>> canvas;

    double wanted_precision = 100.0;

    const double point_opacity = count < 65000 ? 0.6 : count < 650000 ? 0.3 : count < 5000000 ? 0.02 : 0.01;

    for(int i = 0; i < count; i++){
        //first we have to norm transformed acc data
        const double norm_acc = (trs_acc->values[i] - min_acc) / scope;
        // we don't need full precision of double for visualization
        const double x = floor(norm_acc  * 975.0 * wanted_precision) / wanted_precision + 25.0;
        //hr is already normed
        const double y = 975.0 - floor(hr->values[i] * 975.0 * wanted_precision) / wanted_precision;

        //we first put it to map, so we can reduce duplicates
        canvas[x][y] += point_opacity;
    }

    for(const auto& horizontal_pair : canvas){

        const double x = horizontal_pair.first;
        for(const auto& vertical_pair: horizontal_pair.second){
            const double y = vertical_pair.first;
            const double opacity = vertical_pair.second;
            svgFile << "<circle cx=\"" << x << "\" cy=\"" << y << R"(" r="1" opacity=")" << opacity << "\"/>\n";
        }
    }

    // Close the SVG document
    svgFile << "</svg>\n";

    // Close the file
    svgFile.close();

    std::cout << "SVG graph created successfully." << std::endl;
    std::cout << TEXT_SEPARATOR << std::endl;
    return 0;
}

void init_svg_header(std::ofstream &svgFile, const double hr_min, const double hr_max, const double min_acc,
                     const double max_acc) {// Write the SVG header
    svgFile << "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>\n";
    svgFile << "<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\" ";
    svgFile << "\"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\">\n";
    svgFile << "<svg fill=\"#000000\" width=\"1000\" height=\"1000\" xmlns=\"http://www.w3.org/2000/svg\">\n";

    svgFile << "<!-- y axis -->\n";
    svgFile << "<line x1=\"25\" y1=\"0\" x2=\"25\" y2=\"975\" stroke=\"black\"></line>\n";
    svgFile << "<line x1=\"25\" y1=\"0\" x2=\"20\" y2=\"20\" stroke=\"black\"></line>\n";
    svgFile << "<line x1=\"25\" y1=\"0\" x2=\"30\" y2=\"20\" stroke=\"black\"></line>\n";

    svgFile << "<text class=\"heavy\" transform=\"translate(20,550) rotate(-90)\">HeartRate</text>\n";
    svgFile << "<text class=\"heavy\" transform=\"translate(20,970) rotate(-90)\">" << hr_min << "</text>\n";
    svgFile << "<text class=\"heavy\" transform=\"translate(20,50) rotate(-90)\">" << hr_max << "</text>\n";

    svgFile << "<!-- x axis -->\n";
    svgFile << "<line x1=\"25\" y1=\"975\" x2=\"1000\" y2=\"975\" stroke=\"black\"></line>\n";
    svgFile << "<line x1=\"980\" y1=\"970\" x2=\"1000\" y2=\"975\" stroke=\"black\"></line>\n";
    svgFile << "<line x1=\"980\" y1=\"980\" x2=\"1000\" y2=\"975\" stroke=\"black\"></line>\n";

    svgFile << "<text x=\"450\" y=\"990\" class=\"heavy\">Transformed accelerator</text>\n";
    svgFile << R"(<text x="25" y="990" class="heavy">)" << min_acc << "</text>\n";
    svgFile << R"(<text x="930" y="990" class="heavy">)" << max_acc << "</text>\n";
}

void dump_result(const genome& best_genome, const double max_corr){
    std::cout << "_________________________________" << std::endl;
    std::cout << "The best correlation found: " << max_corr << std::endl;
    print_genome(best_genome);
}

void parallel_run(const input_parameters& params){

    //init gpu and check if it is even possible
    std::unique_ptr<OpenCLComponent> cl = nullptr;
    OpenCLComponent::init_opencl_device(cl, params.desired_gpu_name);
    if(cl == nullptr){
        std::cerr << "GPU init failed!" << std::endl;
        exit(1);
    }
    std::cout << TEXT_SEPARATOR << std::endl << std::endl;

    //first we load and preprocess the input files
    ParallelPreprocessor preprocessor {params.input_folder};
    auto input = std::make_shared<input_data>();
    std::cout << TEXT_SEPARATOR << std::endl ;
    preprocessor.load_and_preprocess_folder(input);
    std::cout << TEXT_SEPARATOR << std::endl << std::endl;


    input->acc_entries_count -= input->acc_entries_count % cl->work_group_size;
    input->hr_entries_count -= input->hr_entries_count % cl->work_group_size;

    if(input->acc_entries_count <= 0 || input->hr_entries_count <= 0){
        std::cerr << "Not enough data loaded! Terminating application!" << std::endl;
        exit(1);
    }

    input->acc_x->values.resize(input->acc_entries_count);
    input->acc_y->values.resize(input->acc_entries_count);
    input->acc_z->values.resize(input->acc_entries_count);
    input->hr->values.resize(input->hr_entries_count);

    //then we put the data to genetic algo
    ParallelCalculationScheduler scheduler(*cl, input, params);
    genome best_genome{};
    double max_corr = scheduler.find_transformation_function(best_genome);

    //now dumb the statistics of the best result and plot the correlation into svg
    auto trs_acc = std::make_unique<input_vector>();
    dump_result(best_genome, max_corr);
    scheduler.transform(best_genome);
    trs_acc->values = scheduler.transformation_result;
    preprocessor.find_min_max(trs_acc);
    createSVG(input->hr, trs_acc);
}

void serial_run(const input_parameters& params){
    Preprocessor preprocessor {params.input_folder};

    //first we load and preprocess the input files
    auto input = std::make_shared<input_data>();
    preprocessor.load_and_preprocess_folder(input);
    std::cout << TEXT_SEPARATOR << std::endl << std::endl;

    if(input->acc_entries_count <= 0 || input->hr_entries_count <= 0){
        std::cerr << "Not enough data loaded! Terminating application!" << std::endl;
        exit(1);
    }

    //then we put the data to genetic algo
    CalculationScheduler scheduler(input, params);
    genome best_genome{};
    double max_corr = scheduler.find_transformation_function(best_genome);

    //now dumb the statistics of the best result and plot the correlation into svg
    dump_result(best_genome, max_corr);
    auto trs_acc = std::make_unique<input_vector>();
    scheduler.transform(best_genome);
    trs_acc->values = scheduler.transformation_result;
    preprocessor.find_min_max(trs_acc);

    createSVG(input->hr, trs_acc);
}

inline bool is_quoted(const std::string& s) {
    return s.size() >= 2 && s.front() == '"' && s.back() == '"';
}

inline std::string remove_quotes(const std::string& s) {
    return is_quoted(s) ? s.substr(1, s.size() - 2) : s;
}

/// all possible args input arguments
std::vector<std::string> possible_input_parameters = {"max_step_count", "population_size", "seed",
                                                      "desired_correlation", "const_scope", "pow_scope",
                                                      "gpu_name", "parallel", "step_info_interval"};

input_parameters map_arguments(std::map<size_t, std::string> &arguments, const std::string &input_folder) {
    //first get default values
    size_t max_step_count = DEFAULT_MAX_STEP_COUNT;
    size_t population_size = DEFAULT_POPULATION_SIZE;
    size_t seed = time(nullptr);

    double desired_correlation = DEFAULT_DESIRED_CORRELATION;

    double const_scope = DEFAULT_CONST_SCOPE;
    int pow_scope = DEFAULT_POW_SCOPE;

    std::string desired_gpu_name = DEFAULT_GPU_NAME;

    bool parallel = false;

    size_t step_info_interval = DEFAULT_STEP_INFO_INTERVAL;

    //check if there are any arguments passed that could override default values
    for (const auto& pair : arguments) {
        switch(pair.first){
            case 0:
                max_step_count = abs(std::stoi(pair.second));
                break;
            case 1:
                population_size = abs(std::stoi(pair.second));
                if(population_size < VECTOR_SIZE){
                    std::cerr << "Population size has to be dividable by number " << VECTOR_SIZE << std::endl;
                    exit(-1);
                }
                population_size -= population_size % VECTOR_SIZE;
                break;
            case 2:
                seed = std::stoi(pair.second);
                break;
            case 3:
                desired_correlation = abs(std::stoi(pair.second));
                if(desired_correlation < 0 && desired_correlation > 1){
                    std::cerr << "Desired correlation value has to be between 0 and 1!" << std::endl;
                    exit(-1);
                }
                break;
            case 4:
                const_scope = abs(std::stoi(pair.second));
                if(const_scope > 10000 || const_scope < -10000){
                    std::cerr << "The value you passed for const scope is too great. "
                                 "The calculated sums will probably overflow." << std::endl;
                    exit(-1);
                }
                break;
            case 5:
                pow_scope = abs(std::stoi(pair.second));
                if(pow_scope > 5){
                    std::cerr << "The value you passed for power scope is too great. "
                                 "The calculated sums will probably overflow." << std::endl;
                    exit(-1);
                }else if(pow_scope < 1){
                    std::cerr << "The value you passed for power scope is small. " << std::endl;
                    exit(-1);
                }
                break;
            case 6:
                desired_gpu_name = pair.second;
                break;
            case 7:
                parallel = true;
                break;
            case 8:
                step_info_interval = abs(std::stoi(pair.second));
                break;
        }
    }

    input_parameters params(max_step_count, population_size, seed, desired_correlation, const_scope,
                            pow_scope, desired_gpu_name, parallel, input_folder, step_info_interval);
    return params;
}

void print_help_message() {
    std::cout << "Main usage:" << std::endl;
    std::cout << "application.exe <input_folder>" << std::endl;

    std::cout << "Possible parameters:" << std::endl;
    for (const auto& parameter: possible_input_parameters) {
        if(parameter == "parallel"){
            continue;
        }
        std::cout << "\t-" << parameter << " \"<value>\"" << std::endl;
    }

    std::cout << "Possible flags:" << std::endl;
    std::cout << "\t-parallel" << std::endl;

    std::cout << "More information can be found in documentation." << std::endl;
}

void split_arguments(int argc, char *const *argv, std::map<size_t, std::string> &arguments) {
    // Parse command-line arguments
    for (int i = 2; i < argc; ++i) {
        std::string arg = argv[i];

        // Check if the argument starts with '-'
        if (arg.substr(0, 1) == "-") {
            // Get the argument name
            std::string argName = arg.substr(1);
            auto it = std::find(possible_input_parameters.begin(),
                                possible_input_parameters.end(), argName);
            if(it == possible_input_parameters.end()){
                // Store the argument and its value in the map
                std::cerr << "You tried to insert parameter that is not defined! Terminating the application!"
                                        << std::endl;
                exit(-1);
            }
            size_t index = it - possible_input_parameters.begin();
            // Check if there is a corresponding value
            if (i + 1 < argc) {
                std::string argValue = argv[i + 1];

                // Remove quotes if present
                argValue = remove_quotes(argValue);
                arguments[index] = argValue;
                ++i; // Skip the next element, as it is the value
            } else {
                // Handle case where the argument is a flag without a value
                arguments[index] = "true";
            }
        }
    }
}

input_parameters parse_arguments(int argc, char *const *argv) {
    if(argc < 2){
        print_help_message();
    }

    const std::string input_folder = argv[1];

    // Check if the path exists
    if (!std::filesystem::exists(input_folder)) {
        std::cout << "Path '" << input_folder << "' does not exist or is not accessible." << std::endl;
        exit(-1);
    }
    std::map<size_t, std::string> arguments;
    split_arguments(argc, argv, arguments);
    return map_arguments(arguments, input_folder);
}

int main(int argc, char* argv[]) {
    input_parameters params = parse_arguments(argc, argv);
    params.print_input_parameters();

    std::cout << "Executing code in " << (params.parallel ? "parallel" : "sequential") << std::endl;
    params.parallel ? parallel_run(params) : serial_run(params);
    return EXIT_SUCCESS;
}


#include <iostream>
#include <fstream>
#include "preprocessing/Preprocessor.h"
#include "computation/CalculationScheduler.h"

#include <windows.h>
#include <ppl.h>
#include <algorithm>
#include <array>
#include <valarray>

using namespace concurrency;
using namespace std;

int createSVG(){
    // Open an output file for writing
    std::ofstream svgFile("graph.svg");

    if (!svgFile.is_open()) {
        std::cerr << "Failed to open SVG file." << std::endl;
        return 1;
    }

    // Write the SVG header
    svgFile << "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>\n";
    svgFile << "<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\" ";
    svgFile << "\"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\">\n";
    svgFile << "<svg width=\"300\" height=\"200\" xmlns=\"http://www.w3.org/2000/svg\">\n";

    // Draw a red rectangle
    svgFile << "<rect x=\"50\" y=\"50\" width=\"200\" height=\"100\" fill=\"red\" />\n";

    // Close the SVG document
    svgFile << "</svg>\n";

    // Close the file
    svgFile.close();

    std::cout << "SVG graph created successfully." << std::endl;
    return 0;

}

//todo remove
// Determines whether the input is a prime.
bool is_prime(int n)
{
    if (n < 2)
    {
        return false;
    }

    for (int i = 2; i < int(std::sqrt(n)) + 1; ++i)
    {
        if (n % i == 0)
        {
            return false;
        }
    }
    return true;
}

//todo remove
void doParallelCalc(){
    // Create an array object that contains 200000 integers.
    std::array<int, 2000000> a{};

    // Initialize the array such that a[i] == i.
    int n = 0;
    generate(begin(a), end(a), [&]
    {
        return n++;
    });

    // Use the for_each algorithm to count, serially, the number
    // of prime numbers in the array.
    LONG prime_count = 0L;
    __int64 elapsed = time_call([&]
                                {
                                    for_each(begin(a), end(a), [&](int n)
                                    {
                                        if (is_prime(n))
                                        {
                                            ++prime_count;
                                        }
                                    });
                                });

    std::wcout << L"serial version: " << std::endl
               << L"found " << prime_count << L" prime numbers" << std::endl
               << L"took " << elapsed << L" ms" << std::endl << std::endl;

    // Use the parallel_for_each algorithm to count, in parallel, the number
    // of prime numbers in the array.


    //TODO make sure that the MSVC compiler is present in your project with amd64x instruction set
    prime_count = 0L;
    elapsed = time_call([&]
                        {
                            concurrency::parallel_for_each(begin(a), end(a), [&](int n)
                            {
                                if (is_prime(n))
                                {
                                    InterlockedIncrement(&prime_count);
                                }
                            });
                        });

    std::wcout << L"parallel version: " << std::endl
               << L"found " << prime_count << L" prime numbers" << std::endl
               << L"took " << elapsed << L" ms" << std::endl << std::endl;
}


int main(int argc, char* argv[]) {

    //todo input parameter choose from serial or parallel
    input_parameters params {}; //todo init from args

    Preprocessor preprocessor {};

    auto input = std::make_shared<input_data>();

    //todo folder from parameter
    if(!preprocessor.load_and_preprocess_folder(R"(c:\Users\pulta\OneDrive\Plocha\School\N\5\PPR\data\big-ideas-lab-glycemic-variability-and-wearable-device-data-1.1.2\001\)", input)){
        return EXIT_FAILURE;
    }

    std::cout << "-------------------------------" << std::endl << std::endl;

    CalculationScheduler scheduler{input, params};
    genome best_genome{};
    auto max_corr = scheduler.find_transformation_function(best_genome);
//    auto max_corr = 0.5;

    std::cout << "The best correlation found: " << max_corr << std::endl;

    auto& c = best_genome.constants;
    auto& p = best_genome.powers;
    std::cout << "Best polynomial: "    << c[0] << "x^" << (int)p[0] << " + "
                                        << c[1] << "y^" << (int)p[1] << " + "
                                        << c[2] << "z^" << (int)p[2] << " + "
                                        << c[3];
    return EXIT_SUCCESS;
}

#include <iostream>
#include <fstream>
#include "computation/gpu/opencl_utils.h"
#include "preprocessing/Preprocessor.h"
#include "computation/CalculationScheduler.h"
#include <vector>

#include <windows.h>
#include <ppl.h>
#include <iostream>
#include <algorithm>
#include <array>
#include <valarray>

using namespace concurrency;
using namespace std;

//todo remove
// Returns the number of milliseconds that it takes to call the passed in function.
template <class Function>
__int64 time_call(Function&& f)
{
    __int64 begin = GetTickCount();
    f();
    return GetTickCount() - begin;
}

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

    //todo parser of input parameters
    Preprocessor preprocessor {};

    auto input = std::make_shared<input_data>();

    //todo folder from parameter
    if(!preprocessor.load_and_preprocess("002", input)){
        return EXIT_FAILURE;
    }

    //todo input parameter choose from serial or parallel
    CalculationScheduler scheduler{input};
    find_transformation_function()


    return EXIT_SUCCESS;
}

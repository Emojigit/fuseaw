#include <iostream>
#include <fstream>
#include <vector>
#include "wavescan.h"

// g++ -std=c++23 test_languages.cpp src/wavescan.cpp -I./src -o test_languages

int main(int argc, char* argv[])
{
    // 1. Check command line arguments
    if (argc < 2)
    {
        std::cerr << "Usage: " << argv[0] << " <path_to_akpk_file>\n";
        return 1;
    }

    // 2. Open the file in binary mode
    std::string filename = argv[1];
    std::ifstream file(filename, std::ios::binary);

    if (!file.is_open())
    {
        std::cerr << "Error: Could not open file '" << filename << "'\n";
        return 1;
    }

    // 3. Load and verify the AKPK Header
    AKPKHeader header{};
    if (!load_akpk_header(file, header))
    {
        std::cerr << "Error: Failed to parse AKPK header.\n";
        return 1;
    }

    std::cout << "Successfully loaded AKPK Header.\n";
    std::cout << "Languages Sector Size: " << header.languages_sector_size << " bytes\n";
    std::cout << "---------------------------------------------------------\n";

    // 4. Retrieve the Language Data if the sector is not empty
    if (header.languages_sector_size == 0)
    {
        std::cout << "No language data present in this file.\n";
        return 0;
    }

    std::vector<AKPKLanguageData> language_list;
    if (get_languages(file, header.languages_sector_size, language_list))
    {
        std::cout << "Found " << language_list.size() << " language entries:\n\n";
        std::cout << "Index\tLanguage ID\tOffset\tString Start Offset\n";
        std::cout << "-----\t-----------\t------\t-------------------\n";

        for (size_t i = 0; i < language_list.size(); ++i)
        {
            std::cout << "[" << i << "]\t"
                      << "0x" << std::hex << language_list[i].language_id << "\t\t"
                      << "0x" <<  language_list[i].language_offset << "\t"
                      << "0x" <<  language_list[i].string_start_offset << "\n" << std::dec;
        }
    }
    else
    {
        std::cerr << "Error: Failed to parse language sector data.\n";
        return 1;
    }

    return 0;
}
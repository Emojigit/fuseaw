#include <iostream>
#include <fstream>
#include "wavescan.h"

// g++ -std=c++23 test_wavescan.cpp src/wavescan.cpp -I./src -o test_wavescan

int main(int argc, char* argv[])
{
    // 1. Check if the file argument was provided
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

    // 3. Create a header instance and call your function
    AKPKHeader header{};
    if (load_akpk_header(file, header))
    {
        std::cout << "Successfully loaded AKPK Header!\n";
        std::cout << "-----------------------------------\n";
        std::cout << "Header Size:           " << header.header_size << " bytes\n";
        std::cout << "Endian Flag:           0x" << std::hex << header.endian_flag << std::dec << "\n";
        std::cout << "Languages Sector Size: " << header.languages_sector_size << " bytes\n";
        std::cout << "Banks Sector Size:     " << header.banks_sector_size << " bytes\n";
        std::cout << "Sounds Sector Size:    " << header.sounds_sector_size << " bytes\n";
        std::cout << "Externals Sector Size: " << header.externals_sector_size << " bytes\n";
    }
    else
    {
        std::cerr << "Error: Failed to parse AKPK header. File might be truncated or invalid.\n";
        return 1;
    }

    return 0;
}
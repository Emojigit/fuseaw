#include <iostream>
#include <fstream>
#include <vector>
#include "wavescan.h"
#include <iomanip>

// g++ -std=c++23 test_file_list.cpp src/wavescan.cpp -I./src -o test_file_list

void printAKPKTable(const std::vector<AKPKEntry>& entries) {
    // Define column widths
    const int id_w  = 20;
    const int blk_w = 12;
    const int sz_w  = 15;
    const int off_w = 15;
    const int roff_w = 15;
    const int lang_w = 13;
    const int ext_w = 6;

    // Print Header
    std::cout << std::left
              << std::setw(id_w)   << "File ID"
              << std::setw(blk_w)  << "Block Size"
              << std::setw(sz_w)   << "File Size"
              << std::setw(off_w)  << "File Offset"
              << std::setw(roff_w)  << "Real Offset"
              << std::setw(lang_w) << "Language ID"
              << std::setw(ext_w)  << "Ext" 
              << "\n";

    // Print Separator Line
    std::cout << std::string(id_w + blk_w + sz_w + off_w + lang_w + ext_w, '-') << "\n";

    // Print Rows
    for (const auto& entry : entries) {

        std::cout << std::left
                  << std::setw(id_w)   << std::hex << entry.file_id << std::dec
                  << std::setw(blk_w)  << entry.block_size
                  << std::setw(sz_w)   << entry.file_size
                  << std::setw(off_w)  << std::hex << entry.file_offset << std::dec
                  << std::setw(roff_w)  << std::hex << entry.get_real_offset() << std::dec
                  << std::setw(lang_w) << std::hex << entry.language_id << std::dec
                  << std::setw(ext_w)  << entry.file_extension
                  << "\n";
    }
}

int main(int argc, char *argv[])
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
        std::cout << "Index\tLanguage ID\tString Start Offset\n";
        std::cout << "-----\t-----------\t-------------------\n";

        for (size_t i = 0; i < language_list.size(); ++i)
        {
            std::cout << "[" << i << "]\t"
                      << "0x" << std::hex << language_list[i].language_id << std::dec << "\t\t"
                      << language_list[i].string_start_offset << "\n";
        }
    }
    else
    {
        std::cerr << "Error: Failed to parse language sector data.\n";
        return 1;
    }

    const std::endian endianness = (header.endian_flag == 1) ? std::endian::little : std::endian::big;

    uint32_t bank_version = 0;

    std::vector<AKPKEntry> banks_entries;
    if (get_sector(file, header.banks_sector_size, false, false, "bnk", endianness, bank_version, banks_entries))
    {
        std::cout << "Found " << banks_entries.size() << " bank entries:\n\n";
        printAKPKTable(banks_entries);
    }

    if (bank_version == 0) {
        bank_version = 62;
    }

    std::vector<AKPKEntry> sounds_entries;
    if (get_sector(file, header.sounds_sector_size, true, false, "wem", endianness, bank_version, sounds_entries))
    {
        std::cout << "Found " << sounds_entries.size() << " sound entries:\n\n";
        printAKPKTable(sounds_entries);
    }

    std::vector<AKPKEntry> externals_entries;
    if (get_sector(file, header.externals_sector_size, true, true, "wem", endianness, bank_version, externals_entries))
    {
        std::cout << "Found " << externals_entries.size() << " external entries:\n\n";
        printAKPKTable(externals_entries);
    }

    return 0;
}
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <bit>
#include "wavescan.h"
#include "bnk.h"

// Compilation command:
// g++ -std=c++23 test_parse_bnk.cpp src/*.cpp -I./src -o test_parse_bnk

int main(int argc, char *argv[])
{
    // 1. Check command line arguments
    if (argc < 3)
    {
        std::cerr << "Usage: " << argv[0] << " <path_to_akpk_file> <bnk_file_id>\n";
        return 1;
    }

    std::string pack_filename = argv[1];
    std::string target_id_str = argv[2];

    // Parse the target file ID (handles both decimal and hex strings like 0x12A)
    uint64_t target_id = 0;
    try
    {
        target_id = std::stoull(target_id_str, nullptr, 0);
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: Invalid file ID format '" << target_id_str << "'.\n";
        return 1;
    }

    // 2. Open the pack file in binary mode
    std::ifstream file(pack_filename, std::ios::binary);
    if (!file.is_open())
    {
        std::cerr << "Error: Could not open pack file '" << pack_filename << "'\n";
        return 1;
    }

    // 3. Load and verify the AKPK Header
    AKPKHeader header{};
    if (!load_akpk_header(file, header))
    {
        std::cerr << "Error: Failed to parse AKPK header.\n";
        return 1;
    }

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
    
    bool found = false;
    AKPKEntry target_entry{};
    std::vector<AKPKEntry> current_sector_entries;

    // 4. Scan Banks Sector
    if (header.banks_sector_size > 0)
    {
        if (get_sector(file, header.banks_sector_size, false, false, "bnk", endianness, bank_version, current_sector_entries))
        {
            for (const auto& entry : current_sector_entries)
            {
                if (entry.file_id == target_id)
                {
                    target_entry = entry;
                    found = true;
                    break;
                }
            }
        }
    }

    // 5. Scan Sounds Sector (if not found in banks)
    if (!found && header.sounds_sector_size > 0)
    {
        current_sector_entries.clear();
        if (get_sector(file, header.sounds_sector_size, true, false, "wem", endianness, bank_version, current_sector_entries))
        {
            for (const auto& entry : current_sector_entries)
            {
                if (entry.file_id == target_id)
                {
                    target_entry = entry;
                    found = true;
                    break;
                }
            }
        }
    }

    // 6. Scan Externals Sector (if not found yet)
    if (!found && header.externals_sector_size > 0)
    {
        current_sector_entries.clear();
        if (get_sector(file, header.externals_sector_size, true, true, "wem", endianness, bank_version, current_sector_entries))
        {
            for (const auto& entry : current_sector_entries)
            {
                std::cout << entry.file_id << "\t" << target_id << std::endl;
                if (entry.file_id == target_id)
                {
                    target_entry = entry;
                    found = true;
                    break;
                }
            }
        }
    }

    // 7. Extract the file data if found
    if (!found)
    {
        std::cerr << "Error: File ID " << target_id << " could not be found in any sector.\n";
        return 1;
    }

    std::cout << "Found target entry (Size: " << target_entry.file_size << " bytes). Parsing...\n";

    std::vector<BNKFileMeta> bnk_files;

    if (!parse_bnk(file, target_entry, bnk_files)) {
        std::cerr << "Error: Failed to parse BNK file.\n";
        return 1;
    }

    std::cout << "ID\tOffset\tSize\tGlobal Offset\n";

    for (const auto& file : bnk_files) {
        std::cout << file.meta_raw.wem_id << "\t"
                  << file.meta_raw.wem_offset << "\t"
                  << file.meta_raw.wem_size << "\t"
                  << file.global_offset << "\n";
    }

    return 0;
}
#include "wavescan.h"
#include "helpers.h"

#include <bit>
#include <iostream>
#include <fstream>
#include <vector>
#include <cstdint>
#include <cstring>


bool load_akpk_header(std::istream& file, AKPKHeader& out_header) {
    constexpr std::string_view expected_magic = "AKPK";
    char magic[4];
    
    if (!file.read(magic, sizeof(magic))) {
        std::cerr << "Error: Failed to read magic bytes from file." << std::endl;
        return false;
    }

    if (std::string_view(magic, 4) != expected_magic) {
        std::cerr << "Error: Magic bytes mismatch." << std::endl;
        return false;
    }

    constexpr size_t core_header_size = sizeof(uint32_t) * 5; 
    if (!file.read(reinterpret_cast<char*>(&out_header), core_header_size)) {
        std::cerr << "Error: Failed to read core header fields.\n";
        return false;
    }

    if (out_header.languages_sector_size + out_header.banks_sector_size + out_header.sounds_sector_size + 0x10 < out_header.header_size) {
        file.read(reinterpret_cast<char*>(&out_header.externals_sector_size), sizeof(out_header.externals_sector_size));
    } else {
        out_header.externals_sector_size = 0;
    }

    return true;
}

bool get_languages(std::istream& file, uint32_t languages_sector_size, std::vector<AKPKLanguageData, std::allocator<AKPKLanguageData>>& language_data) {
    const std::streampos language_sector_begin = file.tellg();

    uint32_t total_languages;
    file.read(reinterpret_cast<char*>(&total_languages), sizeof(total_languages));

    for (uint32_t i = 0; i < total_languages; i++) {
        AKPKLanguageData this_language{};
        constexpr size_t language_data_base_size = sizeof(uint32_t) * 2;
        file.read(reinterpret_cast<char*>(&this_language), sizeof(language_data_base_size));
        this_language.string_start_offset = language_sector_begin + static_cast<std::streamoff>(this_language.language_offset);

        const std::streampos current_pos = file.tellg();
        file.seekg(this_language.string_start_offset);

        auto result = read_string_from_stream(file);
        if (result) {
            this_language.language_name = *result;
        } else {
            std::cerr << "Failed to parse language name " << std::hex << this_language.language_id << std::dec << "\n";
        }

        file.seekg(current_pos);

        language_data.push_back(this_language);
    }

    file.seekg(language_sector_begin + static_cast<std::streamoff>(languages_sector_size));

    return true;
}

bool get_sector(
    std::istream& file,
    uint32_t sector_size,
    bool is_sounds,
    bool is_externals,
    const char default_extension[4],
    std::endian endianness,
    uint32_t& bank_version,
    std::vector<AKPKEntry, std::allocator<AKPKEntry>>& sector_files
) {
    if (sector_size == 0) 
        return true;

    uint32_t total_files;
    file.read(reinterpret_cast<char*>(&total_files), sizeof(total_files));
    if (total_files == 0)
        return true;
    
    const uint32_t entry_size = (sector_size - 0x04) / total_files;
    const bool alt_mode = entry_size == 0x18;

    for (uint32_t i = 0; i < total_files; i++) {
        AKPKEntry this_entry{};

        if (alt_mode && is_externals) {
            uint32_t file_ids[2];
            file.read(reinterpret_cast<char*>(file_ids), sizeof(file_ids));

            if (endianness == std::endian::little) {
                std::swap(file_ids[0], file_ids[1]);
            }

            std::memcpy(&this_entry.file_id, file_ids, sizeof(file_ids));
        } else {
            this_entry.file_id = 0;
            file.read(reinterpret_cast<char*>(&this_entry.file_id), sizeof(uint32_t));
        }
        
        file.read(reinterpret_cast<char*>(&this_entry.block_size), sizeof(uint32_t));

        if (alt_mode && !is_externals) {
            file.read(reinterpret_cast<char*>(&this_entry.file_size), sizeof(uint64_t));
        } else {
            this_entry.file_size = 0;
            file.read(reinterpret_cast<char*>(&this_entry.file_size), sizeof(uint32_t));
        }

        file.read(reinterpret_cast<char*>(&this_entry.file_offset), sizeof(uint32_t));
        file.read(reinterpret_cast<char*>(&this_entry.language_id), sizeof(uint32_t));

        const std::streampos offset = this_entry.file_offset * (this_entry.block_size == 0 ? 1 : this_entry.block_size);

        if (!is_sounds && bank_version == 0) {
            const std::streampos original_pos = file.tellg();
            const std::streampos bank_offset = offset + static_cast<std::streamoff>(sizeof(uint32_t) * 2);
            file.seekg(bank_offset);
        
            file.read(reinterpret_cast<char*>(&bank_version), sizeof(uint32_t));

            if (bank_version > 0x1000) {
                bank_version = 62;
            }

            file.seekg(original_pos);
        }

        if (is_sounds && bank_version < 62) {
            const std::streampos original_pos = file.tellg();
            const std::streampos codec_offset = offset + static_cast<std::streamoff>(0x14);
            file.seekg(codec_offset);

            uint16_t codec;
            file.read(reinterpret_cast<char*>(&codec), sizeof(uint16_t));

            switch (codec)
            {
            case 0x0401:
            case 0x0166:
                std::strcpy(this_entry.file_extension, "xma");
                break;
            
            case 0xFFFF:
                std::strcpy(this_entry.file_extension, "ogg");
                break;
            
            default:
                std::strcpy(this_entry.file_extension, "wav");
                break;
            }

            file.seekg(original_pos);
        } else {
            std::strcpy(this_entry.file_extension, default_extension);
        }

        sector_files.push_back(this_entry);
    }

    return true;
}

bool get_file(std::istream& file, AKPKEntry akpk_entry, char* out_buf) {
    const std::streampos offset = akpk_entry.get_real_offset();
    file.seekg(offset);
    file.read(out_buf, akpk_entry.file_size);

    return true;
}

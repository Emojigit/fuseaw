#pragma once

#include <cstdint>
#include <fstream>

#pragma pack(push, 1)

struct AKPKHeader
{
    uint32_t header_size;
    uint32_t endian_flag;
    uint32_t languages_sector_size;
    uint32_t banks_sector_size;
    uint32_t sounds_sector_size;
    uint32_t externals_sector_size;
};

struct AKPKLanguageData
{
    uint32_t language_offset;
    uint32_t language_id;
    std::streampos string_start_offset;
    std::string language_name;
};

#pragma pack(pop)

struct AKPKEntry
{
    uint64_t file_id;
    uint32_t block_size;
    uint64_t file_size;
    uint32_t file_offset;
    uint32_t language_id;
    char file_extension[4];

    inline std::streampos get_real_offset() const {
        return file_offset * (block_size == 0 ? 1 : block_size);
    }
};

bool load_akpk_header(std::istream& file, AKPKHeader& out_header);

bool get_languages(std::istream& file, uint32_t languages_sector_size, std::vector<AKPKLanguageData, std::allocator<AKPKLanguageData>>& language_data);

bool get_sector(
    std::istream& file,
    uint32_t sector_size,
    bool is_sounds,
    bool is_externals,
    const char default_extension[4],
    std::endian endianness,
    uint32_t& bank_version,
    std::vector<AKPKEntry, std::allocator<AKPKEntry>>& sector_files
);

bool get_file(std::istream& file, AKPKEntry bnk_entry, char* out_buf);

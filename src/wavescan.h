#pragma once

#include "common.h"

#include <bit>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#pragma pack(push, 1)

struct AKPKHeader
{
    uint32_t header_size;
    uint32_t endian_flag;
    uint32_t languages_sector_size;
    uint32_t banks_sector_size;
    uint32_t sounds_sector_size;
    uint32_t externals_sector_size;
    uint32_t akpk_header_size;

    inline size_t get_banks_sector_offset() const
    {
        return akpk_header_size + languages_sector_size;
    }

    inline size_t get_sounds_sector_offset() const
    {
        return get_banks_sector_offset() + banks_sector_size;
    }

    inline size_t get_externals_sector_offset() const
    {
        return get_sounds_sector_offset() + sounds_sector_size;
    }
};

struct AKPKLanguageData
{
    uint32_t language_offset;
    uint32_t language_id;
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

    inline size_t get_real_offset() const
    {
        return file_offset * (block_size == 0 ? 1 : block_size);
    }
};

bool load_akpk_header(bytespan_t file, AKPKHeader &out_header);

bool get_languages(
    bytespan_t file,
    size_t language_sector_begin,
    std::vector<AKPKLanguageData, std::allocator<AKPKLanguageData>> &language_data);

bool get_sector(
    bytespan_t file,
    size_t sector_begin,
    uint32_t sector_size,
    bool is_sounds,
    bool is_externals,
    const char default_extension[4],
    std::endian endianness,
    uint32_t &bank_version,
    std::vector<AKPKEntry, std::allocator<AKPKEntry>> &sector_files);

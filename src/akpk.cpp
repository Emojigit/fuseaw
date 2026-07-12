// Function that chains wavescan calls in the correct order and returns everything
// Note that some functions move pointers and depends on the location set by the
// previous function.

#include "akpk.h"

#include <cstring>

#include "bnk.h"
#include "common.h"
#include "wavescan.h"

bool parse_akpk_span(bytespan_t file, AKPKFileData &data)
{
    if (!load_akpk_header(file, data.header))
    {
        return false;
    }

    if (!get_languages(file, data.header.akpk_header_size, data.language_data))
    {
        return false;
    }

    const std::endian endianness = (data.header.endian_flag == 1) ? std::endian::little : std::endian::big;
    uint32_t bank_version = 0;

    if (!get_sector(file, data.header.get_banks_sector_offset(), data.header.banks_sector_size, false, false, "bnk", endianness, bank_version, data.sector_banks))
    {
        return false;
    }

    if (bank_version == 0)
        bank_version = 62;

    if (!get_sector(file, data.header.get_sounds_sector_offset(), data.header.sounds_sector_size, true, false, "wem", endianness, bank_version, data.sector_sounds))
    {
        return false;
    }

    if (!get_sector(file, data.header.get_externals_sector_offset(), data.header.externals_sector_size, true, true, "wem", endianness, bank_version, data.sector_externals))
    {
        return false;
    }

    for (const auto &entry : data.sector_banks)
    {
        if (strcmp(entry.file_extension, "bnk") != 0)
            continue;

        BNKFile bnk_file;

        if (parse_bnk(file, entry, bnk_file))
        {
            // Forgive invalid blocks, we may have hit a HIRC block
            // https://github.com/Escartem/AnimeWwise/blob/c2d9bfc09c679d73466150d8f127b345946cf8e3/bnk.py#L26
            data.bnk_files[entry.file_id] = bnk_file;
        }
    }

    return true;
}

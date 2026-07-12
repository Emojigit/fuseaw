
#include "bnk.h"

#include <cstring>
#include <iostream>
#include <memory>
#include <string_view>

#include "common.h"
#include "wavescan.h"

bool parse_bnk(const bytespan_t file, const AKPKEntry &bnk_entry, BNKFile &bnk_file)
{
    const size_t bnk_offset = bnk_entry.get_real_offset();

    CHECK_OR_RETURN_ERR(file.size() >= bnk_offset + 4, "Error: File too small to contain BKHD magic bytes.");

    std::string_view magic(reinterpret_cast<const char *>(file.subspan(bnk_offset).data()), 4);
    CHECK_OR_RETURN_ERR(magic == "BKHD", "Error: BKHD magic bytes mismatch.");

    uint32_t bkhd_size;
    std::memcpy(&bkhd_size, file.subspan(bnk_offset + 4).data(), sizeof(bkhd_size));
    CHECK_OR_RETURN_ERR(file.size() >= bnk_offset + bkhd_size + 8, "Error: File too small to contain DIDX magic bytes.");

    std::string_view magic_2(reinterpret_cast<const char *>(file.subspan(bnk_offset + 8 + bkhd_size).data()), 4);

    if (magic_2 != "DIDX")
    {
        std::cerr << "Error: DIDX magic bytes mismatch: "
                  << std::hex << bnk_entry.file_id << std::dec;

        // For the sake of debugging: Is it HIRC?
        if (magic_2 == "HIRC")
        {
            std::cerr << " (HIRC?)";
        }

        std::cerr << std::endl;
        return false;
    }

    uint32_t didx_size;
    std::memcpy(&didx_size, file.subspan(bnk_offset + bkhd_size + 12).data(), sizeof(didx_size));
    CHECK_OR_RETURN_ERR(
        file.size() >= bnk_offset + bkhd_size + 16 + didx_size,
        "Error: File too small to contain all DIDX metadata file metadata.");

    const uint32_t n_wems = didx_size / 12;
    const size_t metadata_entries_base = bnk_offset + bkhd_size + 16;
    const size_t global_offset_base = bnk_offset + bkhd_size + 16 + didx_size + 8;

    for (uint32_t i = 0; i < n_wems; i++)
    {
        BNKFileMeta this_meta{};
        const size_t this_offset = metadata_entries_base + (sizeof(BNKFileMetaRaw) * i);

        std::memcpy(&this_meta.meta_raw, file.subspan(this_offset).data(), sizeof(BNKFileMetaRaw));
        this_meta.global_offset = global_offset_base + this_meta.meta_raw.wem_offset;

        bnk_file.push_back(this_meta);
    }

    std::string_view magic_3(reinterpret_cast<const char *>(file.subspan(bnk_offset + bkhd_size + 16 + didx_size).data()), 4);
    CHECK_OR_RETURN_ERR(magic_3 == "DATA", "Error: DATA magic bytes mismatch.");

    return true;
}

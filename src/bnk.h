#pragma once

#include <cstdint>
#include <istream>
#include <vector>
#include <memory>

#include "wavescan.h"

#pragma pack(push, 1)
struct BNKFileMetaRaw
{
    uint32_t wem_id;
    uint32_t wem_offset;
    uint32_t wem_size;
};
#pragma pack(pop)

struct BNKFileMeta
{
    BNKFileMetaRaw meta_raw;
    std::streampos global_offset;
};

bool parse_bnk(std::istream &file, AKPKEntry bnk_entry, std::vector<BNKFileMeta, std::allocator<BNKFileMeta>> &bnk_files);

bool get_bnk_file(std::istream& file, BNKFileMeta bnk_file, char* out_buf);


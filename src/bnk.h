#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include "common.h"
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
    size_t global_offset;
};

bool parse_bnk(bytespan_t file, AKPKEntry bnk_entry, std::vector<BNKFileMeta, std::allocator<BNKFileMeta>> &bnk_files);

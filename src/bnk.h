#pragma once

#include <cstdint>
#include <map>
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

using BNKFile = std::vector<BNKFileMeta, std::allocator<BNKFileMeta>>;
using BNKFileMap = std::map<uint64_t, BNKFile>;

bool parse_bnk(const bytespan_t file, const AKPKEntry &bnk_entry, BNKFile &bnk_file);

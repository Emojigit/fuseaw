#pragma once

#include <cstddef>
#include <map>
#include <vector>

#include "bnk.h"
#include "common.h"
#include "wavescan.h"

struct AKPKFileData {
    AKPKHeader header;
    std::vector<AKPKLanguageData> language_data;
    std::vector<AKPKEntry> sector_banks;
    std::vector<AKPKEntry> sector_sounds;
    std::vector<AKPKEntry> sector_externals;
    std::map<uint64_t, std::vector<BNKFileMeta>> bnk_files;
};

bool parse_akpk_span(bytespan_t file, AKPKFileData &data);

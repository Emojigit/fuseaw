#pragma once

#include <map>
#include <vector>

#include "wavescan.h"
#include "bnk.h"

struct AKPKFileData {
    AKPKHeader header;
    std::vector<AKPKLanguageData> language_data;
    std::vector<AKPKEntry> sector_banks;
    std::vector<AKPKEntry> sector_sounds;
    std::vector<AKPKEntry> sector_externals;
    std::map<uint64_t, std::vector<BNKFileMeta>> bnk_files;
};

bool parse_akpk_file(std::istream &file, AKPKFileData &data);

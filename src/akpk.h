#pragma once

#include <cstddef>

#include "bnk.h"
#include "common.h"
#include "wavescan.h"

struct AKPKFileData {
    AKPKHeader header;
    AKPKLanguageDataList language_data;
    AKPKEntryList sector_banks;
    AKPKEntryList sector_sounds;
    AKPKEntryList sector_externals;
    BNKFileMap bnk_files;
};

bool parse_akpk_span(const bytespan_t file, AKPKFileData &data);

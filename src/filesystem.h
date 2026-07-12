#pragma once

#include <cstddef>
#include <cstdint>
#include <map>
#include <string>

#include "wavescan.h"
#include "bnk.h"
#include "akpk.h"

enum APKPFilesystemType { File, Directory };

struct AKPKFilesystemNode {
    APKPFilesystemType type;
    size_t offset;
    size_t size;
    std::map<std::string, AKPKFilesystemNode> children;
};

bool construct_bnk_filesystem(
    const BNKFile& bnk_contents,
    AKPKFilesystemNode& root
);

bool construct_sector_filesystem(
    const AKPKEntryList& sector_entries,
    const AKPKLanguageDataList& language_data,
    const BNKFileMap& bnk_files,
    AKPKFilesystemNode& root
);

bool construct_akpk_filesystem(
    const AKPKFileData& data,
    AKPKFilesystemNode& root
);

void clean_empty_directories(AKPKFilesystemNode& root);

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
    BNKFile bnk_contents,
    AKPKFilesystemNode& root
);

bool construct_sector_filesystem(
    AKPKEntryList sector_entries,
    AKPKLanguageDataList language_data,
    BNKFileMap bnk_files,
    AKPKFilesystemNode& root
);

bool construct_akpk_filesystem(
    AKPKFileData data,
    AKPKFilesystemNode& root
);

void clean_empty_directories(AKPKFilesystemNode& root);

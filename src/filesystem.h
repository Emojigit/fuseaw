#pragma once

#include <istream>
#include <string>
#include <map>
#include <cstdint>

enum APKPFilesystemType { File, Directory };

struct AKPKFilesystemNode {
    APKPFilesystemType type;
    std::streampos offset;
    std::streamoff size;
    std::map<std::string, AKPKFilesystemNode> children;
};

bool construct_bnk_filesystem(
    std::vector<BNKFileMeta> bnk_contents,
    AKPKFilesystemNode& root
);

bool construct_sector_filesystem(
    std::vector<AKPKEntry> sector_entries,
    std::vector<AKPKLanguageData> language_data,
    std::map<uint64_t, std::vector<BNKFileMeta>> bnk_files,
    AKPKFilesystemNode& root
);

bool construct_akpk_filesystem(
    AKPKFileData data,
    AKPKFilesystemNode& root
);

void clean_empty_directories(AKPKFilesystemNode& root);

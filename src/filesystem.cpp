#include "filesystem.h"

#include <cstdint>
#include <cstring>
#include <format>
#include <ranges>
#include <string>

#include "akpk.h"
#include "bnk.h"
#include "wavescan.h"

bool construct_bnk_filesystem(
    const BNKFile& bnk_contents,
    AKPKFilesystemNode& root
) {
    root.type = Directory;
    for (const auto& entry : bnk_contents) {
        AKPKFilesystemNode this_node{};

        const std::string filename = std::format("0x{:x}.wem", entry.meta_raw.wem_id);
        this_node.type = File;
        this_node.offset = entry.global_offset;
        this_node.size = entry.meta_raw.wem_size;

        root.children[filename] = this_node;
    }

    return true;
}

bool construct_sector_filesystem(
    const AKPKEntryList& sector_entries,
    const AKPKLanguageDataList& language_data,
    const BNKFileMap& bnk_files,
    AKPKFilesystemNode& root
) {
    root.type = Directory;

    std::map<uint32_t, AKPKFilesystemNode*> language_directories;

    for (const auto& [i, lang] : std::views::enumerate((language_data))) {
        AKPKFilesystemNode this_node{};
        this_node.type = Directory;

        auto [it, inserted] = root.children.insert_or_assign(lang.language_name, this_node);
        language_directories[static_cast<uint32_t>(i)] = &it->second;
    }

    for (const auto& entry : sector_entries) {
        AKPKFilesystemNode this_node{};

        const std::string filename = std::format("0x{:x}.{}", entry.file_id, entry.file_extension);

        if (strcmp(entry.file_extension, "bnk") == 0 && bnk_files.contains(entry.file_id)) {
            if (!construct_bnk_filesystem(bnk_files.at(entry.file_id), this_node)) {
                return false;
            }
        } else {
            this_node.type = File;
            this_node.offset = entry.get_real_offset();
            this_node.size = entry.file_size;
        }

        if (language_directories.contains(entry.language_id)) {
            language_directories[entry.language_id]->children[filename] = this_node;
        } else {
            root.children[filename] = this_node;
        }
    }

    return true;
}

bool construct_akpk_filesystem(
    const AKPKFileData& data,
    AKPKFilesystemNode& root
) {
    root.type = Directory;

    root.children["banks"] = AKPKFilesystemNode{};
    if (!construct_sector_filesystem(data.sector_banks, data.language_data, data.bnk_files, root.children["banks"])) {
        return false;
    }

    root.children["sounds"] = AKPKFilesystemNode{};
    if (!construct_sector_filesystem(data.sector_sounds, data.language_data, data.bnk_files, root.children["sounds"])) {
        return false;
    }

    root.children["externals"] = AKPKFilesystemNode{};
    if (!construct_sector_filesystem(data.sector_externals, data.language_data, data.bnk_files, root.children["externals"])) {
        return false;
    }

    return true;
}

void clean_empty_directories(AKPKFilesystemNode& root) {
    if (root.type != Directory) return;

    std::erase_if(root.children, [](auto& item) {
        auto& [key, value] = item;
        
        if (value.type != Directory) return false;
        clean_empty_directories(value);

        return value.children.empty();
    });
}

AKPKFilesystemNode* traverse_node(AKPKFilesystemNode* root, std::string_view path) {
    AKPKFilesystemNode* node = root;

    for (const auto word : path | std::views::split('/')) {
        std::string_view segment(word.begin(), word.end());
        if (segment.empty()) continue;

        auto it = node->children.find(segment);
        if (it == node->children.end()) {
            return nullptr;
        }

        node = &it->second;
    }

    return node;
}

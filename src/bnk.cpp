#include "bnk.h"
#include "wavescan.h"

#include <vector>
#include <cstring>
#include <iostream>

bool parse_bnk(std::istream& file, AKPKEntry bnk_entry, std::vector<BNKFileMeta, std::allocator<BNKFileMeta>>& bnk_files) {
    const std::streampos bnk_offset = bnk_entry.get_real_offset();
    file.clear();
    file.seekg(bnk_offset);

    char magic_1[4];

    if (!file.read(magic_1, sizeof(magic_1))) {
        std::cerr << "Error: Failed to read bkhd signature from file: "
            << std::hex << bnk_entry.file_id << std::dec << std::endl;
        return false;
    }

    if (std::string_view(magic_1, 4) != "\x42\x4B\x48\x44") {
        std::cerr << "Error: bkhd signature mismatch: "
            << std::hex << bnk_entry.file_id << std::dec << std::endl;
        return false;
    }

    uint32_t bkhd_size;
    file.read(reinterpret_cast<char*>(&bkhd_size), sizeof(bkhd_size));
    file.seekg(bkhd_size, std::ios_base::cur);

    char magic_2[4];

    if (!file.read(magic_2, sizeof(magic_2))) {
        std::cerr << "Error: Failed to read didx signature from file: "
            << std::hex << bnk_entry.file_id << std::dec << std::endl;
        return false;
    }

    if (std::string_view(magic_2, 4) != "\x44\x49\x44\x58") {
        std::cerr << "Error: didx signature mismatch: "
            << std::hex << bnk_entry.file_id << std::dec;

        // For the sake of debugging: Is it HIRC?
        if (std::string_view(magic_2, 4) == "HIRC") {
            std::cerr << " (HIRC?)";
        }

        std::cerr << std::endl;
        return false;
    }

    uint32_t didx_size;
    file.read(reinterpret_cast<char*>(&didx_size), sizeof(didx_size));
    const uint32_t n_wems = didx_size / 12;

    const std::streampos global_offset_base = file.tellg() + static_cast<std::streamoff>(didx_size + sizeof(uint32_t) + 4UL);

    for (uint32_t i = 0; i < n_wems; i++) {
        BNKFileMeta this_meta{};

        file.read(reinterpret_cast<char*>(&this_meta.meta_raw), sizeof(this_meta.meta_raw));

        this_meta.global_offset = global_offset_base + static_cast<std::streamoff>(this_meta.meta_raw.wem_offset);

        bnk_files.push_back(this_meta);
    }

    char magic_3[4];

    if (!file.read(magic_3, sizeof(magic_3))) {
        std::cerr << "Error: Failed to read data signature from file: "
            << std::hex << bnk_entry.file_id << std::dec << std::endl;
        return false;
    }

    if (std::string_view(magic_3, 4) != "\x44\x41\x54\x41") {
        std::cerr << "Error: data signature mismatch: "
            << std::hex << bnk_entry.file_id << std::dec << std::endl;
        return false;
    }

    return true;
}

bool get_bnk_file(std::istream& file, BNKFileMeta bnk_file, char* out_buf) {
    file.seekg(bnk_file.global_offset);
    file.read(out_buf, bnk_file.meta_raw.wem_size);

    return true;
}

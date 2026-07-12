#include "wavescan.h"

#include <bit>
#include <cstring>
#include <format>
#include <iostream>
#include <string_view>

#include "common.h"
#include "helpers.h"

bool load_akpk_header(bytespan_t file, AKPKHeader &out_header)
{
    CHECK_OR_RETURN_ERR(file.size() >= 24, "Error: File too small to containing AKPK header.");

    std::string_view magic(reinterpret_cast<const char *>(file.data()), 4);
    CHECK_OR_RETURN_ERR(magic == "AKPK", "Error: AKPK magic bytes mismatch.");

    std::memcpy(&out_header, file.subspan(4, 20).data(), 20);

    if (out_header.languages_sector_size + out_header.banks_sector_size + out_header.sounds_sector_size + 0x10 < out_header.header_size)
    {
        CHECK_OR_RETURN_ERR(file.size() >= 28, "Error: File too small to containing external sector size.");
        std::memcpy(&out_header.externals_sector_size,
                    file.subspan(24).data(),
                    sizeof(out_header.externals_sector_size));
        out_header.akpk_header_size = 28;
    }
    else
    {
        out_header.externals_sector_size = 0;
        out_header.akpk_header_size = 24;
    }

    return true;
}

bool get_languages(
    bytespan_t file,
    size_t language_sector_begin,
    AKPKLanguageDataList &language_data)
{
    uint32_t total_languages;
    CHECK_OR_RETURN_ERR(file.size() >= language_sector_begin + sizeof(total_languages), "Error: File too small to contain total language count.");
    std::memcpy(&total_languages, file.subspan(language_sector_begin).data(), sizeof(total_languages));

    constexpr size_t language_data_base_size = sizeof(uint32_t) * 2;
    CHECK_OR_RETURN_ERR(
        file.size() >= language_sector_begin + sizeof(total_languages) + total_languages * language_data_base_size,
        "Error: File too small to contain the language header.");

    for (uint32_t i = 0; i < total_languages; i++)
    {
        AKPKLanguageData this_language{};
        const uint32_t this_language_offset = language_sector_begin + sizeof(total_languages) + sizeof(uint32_t) * 2 * i;
        std::memcpy(&this_language, file.subspan(this_language_offset, language_data_base_size).data(), language_data_base_size);

        const size_t string_start_offset = language_sector_begin + this_language.language_offset;

        auto result = read_string_from_span(file, string_start_offset);
        if (result)
        {
            this_language.language_name = *result;
        }
        else
        {
            std::cerr << "Failed to parse language name " << std::hex << this_language.language_id << std::dec << "\n";
            this_language.language_name = std::format("0x{:x}", this_language.language_id);
        }

        language_data.push_back(this_language);
    }

    return true;
}

bool get_sector(
    bytespan_t file,
    size_t sector_begin,
    uint32_t sector_size,
    bool is_sounds,
    bool is_externals,
    const char default_extension[4],
    std::endian endianness,
    uint32_t &bank_version,
    AKPKEntryList &sector_files)
{
    // Size == 4 -> total_files == 0, don't waste time and declare success
    // If we do have files, it would be an error, and we are not intrested in
    // grabbing data from an errorous state
    if (sector_size <= 4)
        return true;

    CHECK_OR_RETURN_ERR(file.size() >= sector_begin + sizeof(uint32_t), "Error: File too small for sector file count.");

    uint32_t total_files;
    std::memcpy(&total_files, file.subspan(sector_begin).data(), sizeof(total_files));

    const uint32_t entry_size = (sector_size - 0x04) / total_files;
    const bool alt_mode = entry_size == 0x18;

    CHECK_OR_RETURN_ERR(file.size() >= sector_begin + sizeof(uint32_t) + (total_files * entry_size),
                        "Error: File too small to contain all sector metadata entries.");

    for (uint32_t i = 0; i < total_files; i++)
    {
        AKPKEntry this_entry{};
        size_t this_offset = sector_begin + sizeof(total_files) + entry_size * i;

        if (alt_mode && is_externals)
        {
            std::memcpy(&this_entry.file_id, file.subspan(this_offset).data(), sizeof(uint64_t));

            if (endianness == std::endian::little)
            {
                this_entry.file_id = (this_entry.file_id >> 32) | (this_entry.file_id << 32);
            }

            this_offset += sizeof(this_entry.file_id);
        }
        else
        {
            this_entry.file_id = 0;
            std::memcpy(&this_entry.file_id, file.subspan(this_offset).data(), sizeof(uint32_t));

            this_offset += sizeof(uint32_t);
        }

        std::memcpy(&this_entry.block_size, file.subspan(this_offset).data(), sizeof(this_entry.block_size));
        this_offset += sizeof(this_entry.block_size);

        if (alt_mode && !is_externals)
        {
            std::memcpy(&this_entry.file_size, file.subspan(this_offset).data(), sizeof(uint64_t));
            this_offset += sizeof(uint64_t);
        }
        else
        {
            this_entry.file_size = 0;
            std::memcpy(&this_entry.file_size, file.subspan(this_offset).data(), sizeof(uint32_t));
            this_offset += sizeof(uint32_t);
        }

        std::memcpy(&this_entry.file_offset, file.subspan(this_offset).data(), sizeof(this_entry.file_offset));
        this_offset += sizeof(this_entry.file_offset);

        std::memcpy(&this_entry.language_id, file.subspan(this_offset).data(), sizeof(this_entry.language_id));
        this_offset += sizeof(this_entry.language_id);

        const size_t real_offset = this_entry.get_real_offset();

        if (!is_sounds && bank_version == 0)
        {
            const size_t bank_offset = real_offset + sizeof(uint32_t) * 2;

            if (file.size() >= bank_offset + sizeof(uint32_t))
            {
                std::memcpy(&bank_version, file.subspan(bank_offset).data(), sizeof(bank_version));

                if (bank_version > 0x1000)
                {
                    bank_version = 62;
                }
            }
        }

        std::strcpy(this_entry.file_extension, default_extension);

        if (is_sounds && bank_version < 62)
        {
            const size_t codec_offset = real_offset + 0x14;

            if (file.size() >= codec_offset + sizeof(uint16_t))
            {
                uint16_t codec;
                std::memcpy(&codec, file.subspan(codec_offset).data(), sizeof(codec));

                switch (codec)
                {
                case 0x0401:
                case 0x0166:
                    std::strcpy(this_entry.file_extension, "xma");
                    break;
                case 0xFFFF:
                    std::strcpy(this_entry.file_extension, "ogg");
                    break;
                default:
                    std::strcpy(this_entry.file_extension, "wav");
                    break;
                }
            }
        }

        sector_files.push_back(this_entry);
    }

    return true;
}

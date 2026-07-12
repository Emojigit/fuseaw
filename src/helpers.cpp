#include "helpers.h"

#include <cstdint>
#include <expected>
#include <optional>
#include <string>
#include <system_error>

#include "common.h"

// 1. utf-8 / utf-16le helpers, for language names, AI-generated

// Heuristic to guess encoding if the stream supports seeking
StringEncoding detect_encoding(const bytespan_t file) {
    // If we have fewer than 2 bytes, we can't reliably check for UTF-16LE.
    // Default to UTF-8.
    if (file.size() < 2) {
        return StringEncoding::Utf8;
    }

    char b1 = static_cast<char>(file[0]);
    char b2 = static_cast<char>(file[1]);

    // Heuristic: If the second byte is null and the first is not, 
    // it's highly likely a UTF-16LE string containing ASCII/Latin text.
    if (b2 == '\0' && b1 != '\0') {
        return StringEncoding::Utf16Le;
    }
    
    return StringEncoding::Utf8;
}

std::expected<std::string, std::error_code> read_string_from_span(
    const bytespan_t file,
    const size_t start,
    const std::optional<StringEncoding> forced_encoding
) {
    if (start >= file.size()) {
        return std::unexpected(std::make_error_code(std::errc::invalid_argument));
    }

    bytespan_t data = file.subspan(start);
    size_t cursor = 0;

    StringEncoding encoding = forced_encoding.value_or(detect_encoding(data));
    std::string result;

    if (encoding == StringEncoding::Utf8) {
        while (cursor < data.size()) {
            char ch = static_cast<char>(data[cursor++]);
            if (ch == '\0') {
                return result;
            }
            result.push_back(ch);
        }
        if (!result.empty()) return result;
        return std::unexpected(std::make_error_code(std::errc::illegal_byte_sequence));
    } 
    
    // Process UTF-16LE string
    while (true) {
        if (cursor == data.size()) {
            break; // Clean EOF at character boundary
        }
        if (cursor + 2 > data.size()) {
            return std::unexpected(std::make_error_code(std::errc::illegal_byte_sequence)); // Truncated character
        }

        uint8_t b1 = static_cast<uint8_t>(data[cursor++]);
        uint8_t b2 = static_cast<uint8_t>(data[cursor++]);

        // Reassemble little-endian code unit safely avoiding sign extension
        uint16_t u16 = static_cast<uint8_t>(b1) | (static_cast<uint8_t>(b2) << 8);
        if (u16 == 0) {
            break; // Null terminator reached
        }

        uint32_t cp = u16;

        // Handle UTF-16 surrogate pairs
        if (cp >= 0xD800 && cp <= 0xDBFF) { // High surrogate
            if (cursor + 2 > data.size()) {
                return std::unexpected(std::make_error_code(std::errc::illegal_byte_sequence));
            }
            uint8_t b3 = static_cast<uint8_t>(data[cursor++]);
            uint8_t b4 = static_cast<uint8_t>(data[cursor++]);
            
            uint16_t next_u16 = b3 | (b4 << 8);
            if (next_u16 >= 0xDC00 && next_u16 <= 0xDFFF) { // Low surrogate
                cp = 0x10000 + ((cp - 0xD800) << 10) + (next_u16 - 0xDC00);
            } else {
                return std::unexpected(std::make_error_code(std::errc::illegal_byte_sequence));
            }
        } else if (cp >= 0xDC00 && cp <= 0xDFFF) {
            // Unpaired low surrogate is invalid
            return std::unexpected(std::make_error_code(std::errc::illegal_byte_sequence));
        }

        // Convert Unicode code point to UTF-8 bytes
        if (cp <= 0x7F) {
            result.push_back(static_cast<char>(cp));
        } else if (cp <= 0x7FF) {
            result.push_back(static_cast<char>(0xC0 | ((cp >> 6) & 0x1F)));
            result.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        } else if (cp <= 0xFFFF) {
            result.push_back(static_cast<char>(0xE0 | ((cp >> 12) & 0x0F)));
            result.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
            result.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        } else if (cp <= 0x10FFFF) {
            result.push_back(static_cast<char>(0xF0 | ((cp >> 18) & 0x07)));
            result.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
            result.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
            result.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        } else {
            return std::unexpected(std::make_error_code(std::errc::illegal_byte_sequence));
        }
    }

    return result;
}
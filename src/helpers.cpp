#include "helpers.h"

#include <iostream>
#include <string>
#include <expected>
#include <system_error>
#include <cstdint>

// 1. utf-8 / utf-16le helpers, for language names, AI-generated

// Heuristic to guess encoding if the stream supports seeking
StringEncoding detect_encoding(std::istream& stream) {
    auto pos = stream.tellg();
    if (pos == std::streampos(-1)) {
        // Stream doesn't support seeking (e.g., pipe/socket), default to UTF-8
        return StringEncoding::Utf8;
    }

    char b1 = 0, b2 = 0;
    stream.get(b1);
    stream.get(b2);
    
    stream.clear();
    stream.seekg(pos);

    // Heuristic: If the second byte is null and the first is not, 
    // it's highly likely a UTF-16LE string containing ASCII/Latin text.
    if (b2 == '\0' && b1 != '\0') {
        return StringEncoding::Utf16Le;
    }
    
    return StringEncoding::Utf8;
}

std::expected<std::string, std::error_code> read_string_from_stream(
    std::istream& stream, 
    std::optional<StringEncoding> forced_encoding
) {
    StringEncoding encoding = forced_encoding.value_or(detect_encoding(stream));
    std::string result;

    if (encoding == StringEncoding::Utf8) {
        char ch;
        while (stream.get(ch)) {
            if (ch == '\0') {
                return result;
            }
            result.push_back(ch);
        }
        if (stream.eof() && !result.empty()) return result;
        return std::unexpected(std::make_error_code(std::errc::io_error));
    } 
    
    // Process UTF-16LE string
    while (true) {
        char b1, b2;
        if (!stream.get(b1)) {
            if (stream.eof()) break;
            return std::unexpected(std::make_error_code(std::errc::io_error));
        }
        if (!stream.get(b2)) {
            return std::unexpected(std::make_error_code(std::errc::illegal_byte_sequence));
        }

        // Reassemble little-endian code unit safely avoiding sign extension
        uint16_t u16 = static_cast<uint8_t>(b1) | (static_cast<uint8_t>(b2) << 8);
        if (u16 == 0) {
            break; // Null terminator reached
        }

        uint32_t cp = u16;

        // Handle UTF-16 surrogate pairs
        if (cp >= 0xD800 && cp <= 0xDBFF) { // High surrogate
            char b3, b4;
            if (!stream.get(b3) || !stream.get(b4)) {
                return std::unexpected(std::make_error_code(std::errc::illegal_byte_sequence));
            }
            uint16_t next_u16 = static_cast<uint8_t>(b3) | (static_cast<uint8_t>(b4) << 8);
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
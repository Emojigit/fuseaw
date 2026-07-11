#include <iostream>
#include <string>
#include <expected>
#include <system_error>
#include <cstdint>

// 1. utf-8 / utf-16le helpers, for language names, AI-generated

enum class StringEncoding {
    Utf8,
    Utf16Le
};

StringEncoding detect_encoding(std::istream& stream);

std::expected<std::string, std::error_code> read_string_from_stream(
    std::istream& stream, 
    std::optional<StringEncoding> forced_encoding = std::nullopt
);

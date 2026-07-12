#pragma once

#include <expected>
#include <optional>
#include <string>
#include <system_error>

#include "common.h"

// 1. utf-8 / utf-16le helpers, for language names, AI-generated

enum class StringEncoding {
    Utf8,
    Utf16Le
};

StringEncoding detect_encoding(bytespan_t file);

std::expected<std::string, std::error_code> read_string_from_span(
    bytespan_t file,
    size_t start,
    std::optional<StringEncoding> forced_encoding = std::nullopt
);

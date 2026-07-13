#pragma once

#include <expected>
#include <string>
#include <system_error>

#include "common.h"

// 1. utf-8 / utf-16le helpers, for language names, AI-generated

enum class StringEncoding {
    Utf8,
    Utf16Le
};

std::expected<std::string, std::error_code> read_string_from_span(
    const bytespan_t file,
    const size_t start
);

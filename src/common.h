#pragma once

#include <cstddef>
#include <iostream>
#include <span>

#define CHECK_OR_RETURN_ERR(condition, err) if (!(condition)) { std::cerr << err << std::endl; return false; }

using bytespan_t = std::span<const std::byte>;

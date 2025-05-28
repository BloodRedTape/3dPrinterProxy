#pragma once

#include "pch/std.hpp"

struct Base64 {

	static std::string Encode(const std::string &in);

	static std::string Decode(const std::string &in);
};
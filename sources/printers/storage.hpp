#pragma once

#include "pch/std.hpp"

#define WITH_PRINTER_DEBUG 1

struct PrinterStorage {
	virtual void UploadGCodeFileAsync(const std::string &filename, const std::string& content, bool print, std::function<void(bool)> callback) = 0;
};
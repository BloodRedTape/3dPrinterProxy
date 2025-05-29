#pragma once

#include "pch/std.hpp"
#include "printers/file.hpp"

#define WITH_PRINTER_DEBUG 0

struct PrinterStorage {
	virtual void UploadGCodeFileAsync(const std::string &filename, const std::string& content, bool print, std::function<void(bool)> callback) = 0;

	virtual const GCodeFileMetadata *GetMetadata(const std::string &filename)const{ return nullptr; };
};
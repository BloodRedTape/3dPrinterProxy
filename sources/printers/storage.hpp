#pragma once

#include "pch/std.hpp"
#include "printers/file.hpp"
#include "printers/history.hpp"

#define WITH_PRINTER_DEBUG 0

struct PrinterStorage {
	virtual void UploadGCodeFileAsync(const std::string &filename, const std::string& content, bool print, std::function<void(bool)> callback) = 0;

	virtual bool UploadGCodeFile(const std::string &filename, const std::string& content, bool print) = 0;

	virtual const GCodeFileMetadata *GetMetadata(const std::string &filename)const{ return nullptr; };

	virtual const GCodeFileMetadata *GetMetadata(std::size_t content_hash)const{ return nullptr; };

	virtual std::optional<std::size_t> GetContentHashForFilename(const std::string &filename)const{ return std::nullopt; };
};
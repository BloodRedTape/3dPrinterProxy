#pragma once

#include "pch/std.hpp"
#include <bsl/enum.hpp>
#include "printers/file.hpp"
#include "printers/history.hpp"
#include "printers/state.hpp"

#define WITH_PRINTER_DEBUG 0

BSL_ENUM(PrinterStorageUploadStatus,
	Sending,
	Success,
	Failure
);

struct PrinterStorageUploadState {
	std::int32_t Current = 0;
	std::int32_t Target = 0;
	std::string Filename;
	PrinterStorageUploadStatus Status = PrinterStorageUploadStatus::Sending;

	PrinterStorageUploadState(const std::string &filename, std::int32_t current = 0, std::int32_t target = 0):
		Filename(filename),
		Current(current),
		Target(target)
	{}
};

struct PrinterStorage {
	std::function<void()> OnUploadStateChanged;

	virtual std::optional<PrinterStorageUploadState> GetUploadState()const = 0;

	virtual void UploadGCodeFileAsync(const std::string &filename, const std::string& content, bool print, std::function<void(bool)> callback) = 0;

	virtual bool UploadGCodeFile(const std::string &filename, const std::string& content, bool print) = 0;

	virtual const GCodeFileMetadata *GetMetadata(const std::string &filename)const{ return nullptr; };

	virtual const GCodeFileMetadata *GetMetadata(std::size_t content_hash)const{ return nullptr; };

	virtual std::optional<std::size_t> GetContentHashForFilename(const std::string &filename)const{ return std::nullopt; };
};
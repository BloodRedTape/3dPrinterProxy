#pragma once

#include "pch/std.hpp"
#include "pch/json.hpp"

using UnixTime = std::uint64_t;

struct HistoryEntry {
	std::string Filename;
	std::size_t ContentHash = 0;
	UnixTime PrintStart = 0;
	UnixTime PrintEnd = 0;

	NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(HistoryEntry, Filename, ContentHash, PrintStart, PrintEnd)
};

class PrinterHistory {
public:
	virtual const std::vector<HistoryEntry> &GetHistory()const = 0;
};
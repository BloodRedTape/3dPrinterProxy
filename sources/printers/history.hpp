#pragma once

#include <string>

using DateTime = std::int64_t;

struct HistoryEntry {
	std::size_t Hash;
	DateTime PrintStart;
	DateTime PrintEnd;
};

class PrinterHistory {
	
	virtual const std::vector<HistoryEntry> &GetHistory()const = 0;
};
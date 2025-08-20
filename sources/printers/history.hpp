#pragma once

#include "pch/std.hpp"
#include "pch/json.hpp"

using UnixTime = std::uint64_t;

struct HistoryEntry {
	std::string Filename;
	std::size_t ContentHash = 0;
	std::string FileId;
	UnixTime PrintStart = 0;
	UnixTime PrintEnd = 0;

	NLOHMANN_DEFINE_TYPE_INTRUSIVE_ONLY_SERIALIZE(HistoryEntry, Filename, FileId, PrintStart, PrintEnd)

	friend void from_json(const nlohmann::json& json, HistoryEntry& entry) {
		entry.Filename = json["Filename"];
		entry.FileId = json.count("FileId") ? json["FileId"] : "";
		entry.PrintStart = json["PrintStart"];
		entry.PrintEnd = json["PrintEnd"];

		if(json.count("ContentHash"))
			entry.FileId = ToString(std::size_t(json["ContentHash"]));
	}
};

class PrinterHistory {
public:
	virtual const std::vector<HistoryEntry> &GetHistory()const = 0;
};
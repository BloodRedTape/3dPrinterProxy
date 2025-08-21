#pragma once

#include "pch/std.hpp"
#include "pch/json.hpp"
#include <bsl/enum.hpp>
#include "printers/state.hpp"

using UnixTime = std::uint64_t;

BSL_ENUM(PrintFinishReason,
	Unknown,
	Complete,
	Interrupted
);

NLOHMANN_DEFINE_BSL_ENUM_WITH_DEFAULT(PrintFinishReason, PrintFinishReason::Unknown)

struct HistoryEntry {
	std::string Filename;
	std::size_t ContentHash = 0;
	std::string FileId;
	UnixTime PrintStart = 0;
	UnixTime PrintEnd = 0; 
	PrintState LastKnownPrintState;
	PrintFinishReason FinishReason = PrintFinishReason::Complete;

	NLOHMANN_DEFINE_TYPE_INTRUSIVE_ONLY_SERIALIZE(HistoryEntry, Filename, FileId, PrintStart, PrintEnd, LastKnownPrintState, FinishReason)

	friend void from_json(const nlohmann::json& json, HistoryEntry& entry) {
		entry.Filename = json["Filename"];
		entry.FileId = json.count("FileId") ? json["FileId"] : "";
		entry.PrintStart = json["PrintStart"];
		entry.PrintEnd = json["PrintEnd"];

		if(json.count("ContentHash"))
			entry.FileId = ToString(std::size_t(json["ContentHash"]));

		entry.LastKnownPrintState = json["LastKnownPrintState"];
		entry.FinishReason = PrintFinishReason::FromString(json["FinishReason"].get<std::string>()).value_or(PrintFinishReason::Complete);
	}
};

class PrinterHistory {
public:
	virtual const std::vector<HistoryEntry> &GetHistory()const = 0;
};
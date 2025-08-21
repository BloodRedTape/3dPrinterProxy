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

struct PrintFinishState{
	float Progress = 0.f;
	std::int64_t Bytes = 0;
	std::int64_t Layer = 0;
	float Height = 0.f;
	PrintFinishReason Reason = PrintFinishReason::Complete;

	NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(PrintFinishState, Progress, Bytes, Layer, Height, Reason);
};

struct HistoryEntry {
	std::string Filename;
	std::string FileId;
	UnixTime PrintStart = 0;
	UnixTime PrintEnd = 0; 
	PrintFinishState FinishState;

	NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(HistoryEntry, Filename, FileId, PrintStart, PrintEnd, FinishState)
};

class PrinterHistory {
public:
	virtual const std::vector<HistoryEntry> &GetHistory()const = 0;
};
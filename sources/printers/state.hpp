#pragma once

#include "pch/std.hpp"
#include <bsl/enum.hpp>
#include "pch/json.hpp"

BSL_ENUM(PrintStatus,
	Heating,
	Busy,
	Printing
);

NLOHMANN_DEFINE_BSL_ENUM_WITH_DEFAULT(PrintStatus, PrintStatus::Busy)

struct PrintState {
	std::string Filename;
	float Progress = 0.f;
	std::int64_t CurrentBytesPrinted = 0;
	std::int64_t TargetBytesPrinted = 0;
	std::int64_t Layer = 0;
	float Height = 0.f;
	PrintStatus Status = PrintStatus::Busy;
	//Remaining time

	NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(PrintState, Filename, Progress, CurrentBytesPrinted, TargetBytesPrinted, Layer, Height, Status);
};

struct PrinterState {
	float BedTemperature = 0.f;
	float TargetBedTemperature = 0.f;

	float ExtruderTemperature = 0.f;
	float TargetExtruderTemperature = 0.f;

	float FeedRate = 0.f;
	
	//float FanSpeed = 0.f;

	std::optional<PrintState> Print;
};


#pragma once

#include "pch/std.hpp"

enum class PrintStatus {
	Heating,
	Busy,
	Printing,
};

inline std::string to_string(PrintStatus status) {
	if(status == PrintStatus::Heating)
		return "Heating";
	if(status == PrintStatus::Busy)
		return "Busy";
	if(status == PrintStatus::Printing)
		return "Printing";
	return "__None__";
}

struct PrintState {
	std::string Filename;
	float Progress = 0.f;
	std::int64_t CurrentBytesPrinted = 0;
	std::int64_t TargetBytesPrinted = 0;
	std::int64_t Layer = 0;
	float Height = 0.f;
	PrintStatus Status = PrintStatus::Busy;
	//Remaining time
};

struct PrinterState {
	float BedTemperature = 0.f;
	float TargetBedTemperature = 0.f;

	float ExtruderTemperature = 0.f;
	float TargetExtruderTemperature = 0.f;
	
	//float FanSpeed = 0.f;

	std::optional<PrintState> Print;
};


#pragma once

#include "pch/std.hpp"

enum class PrintStatus {
	Busy,
	Printing,
};

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


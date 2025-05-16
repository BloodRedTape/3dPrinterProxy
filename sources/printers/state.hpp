#pragma once

#include <string>
#include <optional>
#include <functional>

enum class PrintStatus {
	Busy,
	Printing,
	//Paused
};

struct PrintState {
	std::string Filename;
	float Progress = 0.f;
	std::int64_t CurrentBytesPrinted = 0;
	std::int64_t TargetBytesPrinted = 0;
	PrintStatus Status = PrintStatus::Busy;
	//Remaining time
};

struct PrinterState {
	float BedTemperature = 0.f;
	float TargetBedTemperature = 0.f;

	float ExtruderTemperature = 0.f;
	float TargetExtruderTemperature = 0.f;
	
	//float FanSpeed;

	std::optional<PrintState> Print;
};


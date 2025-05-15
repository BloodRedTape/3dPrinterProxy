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
	PrintStatus Status = PrintStatus::Busy;
	//Remaining time
};

struct PrinterState {
	float BedTemperature;
	float TargetBedTemperature;

	float ExtruderTemperature;
	float TargetExtruderTemperature;
	
	//float FanSpeed;

	std::optional<PrintState> Print;
};


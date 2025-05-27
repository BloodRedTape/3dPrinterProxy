#include "printer.hpp"
#include <bsl/log.hpp>

DEFINE_LOG_CATEGORY(Printer);

void Printer::DefaultGCodeCallback(GCodeResult result){
	if(result == GCodeResult::Ok)
		return;

	LogPrinter(Error, "GCode failed with %", result.Name());
}

void Printer::IdentifyAsync(GCodeCallback callback){
	callback(GCodeResult::Unsupported);
}

void Printer::SetTargetBedTemperatureAsync(std::int64_t , GCodeCallback callback){
	callback(GCodeResult::Unsupported);
}

void Printer::SetTargetExtruderTemperatureAsync(std::int64_t , GCodeCallback callback){
	callback(GCodeResult::Unsupported);
}

void Printer::SetFeedRatePercentAsync(float , GCodeCallback callback){
	callback(GCodeResult::Unsupported);
}

void Printer::SetLCDMessageAsync(std::string , GCodeCallback callback){
	callback(GCodeResult::Unsupported);
}

void Printer::SetDialogMessageAsync(std::string , std::optional<int> , GCodeCallback callback){
	callback(GCodeResult::Unsupported);
}

void Printer::SetFanSpeedAsync(std::uint8_t , GCodeCallback callback){
	callback(GCodeResult::Unsupported);
}

void Printer::PauseUntillUserInputAsync(std::string , GCodeCallback callback){
	callback(GCodeResult::Unsupported);
}

void Printer::PausePrintAsync(GCodeCallback callback){
	callback(GCodeResult::Unsupported);
}

void Printer::ResumePrintAsync(GCodeCallback callback){
	callback(GCodeResult::Unsupported);
}

void Printer::ReleaseMotorsAsync(GCodeCallback callback){
	callback(GCodeResult::Unsupported);
}

void Printer::CancelPrintAsync(GCodeCallback callback){
	callback(GCodeResult::Unsupported);
}

#pragma once

#include <bsl/enum.hpp>
#include "pch/std.hpp"
#include "storage.hpp"
#include "history.hpp"
#include "printers/state.hpp"

BSL_ENUM(GCodeResult,
	Ok,
	Unsupported,
	NoConnection,
	Busy
);

using GCodeCallback = std::function<void(GCodeResult)>;

class Printer {
public:
	std::function<void()> OnStateChanged;
public:
	static void DefaultGCodeCallback(GCodeResult result);

	virtual void RunAsync() = 0;

	virtual void IdentifyAsync(GCodeCallback callback = DefaultGCodeCallback);

	virtual void SetTargetBedTemperatureAsync(std::int64_t tempearture, GCodeCallback callback = DefaultGCodeCallback);

	virtual void SetTargetExtruderTemperatureAsync(std::int64_t tempearture, GCodeCallback callback = DefaultGCodeCallback);

	virtual void SetFeedRatePercentAsync(float feed_rate, GCodeCallback callback = DefaultGCodeCallback);

	virtual void SetLCDMessageAsync(std::string message, GCodeCallback callback = DefaultGCodeCallback);

	virtual void SetDialogMessageAsync(std::string message, std::optional<int> display_time_seconds = std::nullopt, GCodeCallback callback = DefaultGCodeCallback);

	virtual void SetFanSpeedAsync(std::uint8_t speed, GCodeCallback callback = DefaultGCodeCallback);

	virtual void PauseUntillUserInputAsync(std::string message = "", GCodeCallback callback = DefaultGCodeCallback);

	virtual void PausePrintAsync(GCodeCallback callback = DefaultGCodeCallback);

	virtual void ResumePrintAsync(GCodeCallback callback = DefaultGCodeCallback);

	virtual void ReleaseMotorsAsync(GCodeCallback callback = DefaultGCodeCallback);

	virtual void CancelPrintAsync(GCodeCallback callback = DefaultGCodeCallback);

	virtual bool IsConnected()const = 0;

	virtual PrinterStorage &Storage() = 0;

	virtual std::optional<PrinterState> GetPrinterState()const = 0;
};
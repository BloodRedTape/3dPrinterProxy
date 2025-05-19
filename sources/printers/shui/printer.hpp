#pragma once

#include "pch/std.hpp"
#include "printers/state.hpp"
#include "printers/shui/connection.hpp"
#include "printers/shui/storage.hpp"

namespace boost::asio { class io_context; }

class ShuiPrinter {
	std::filesystem::path m_DataPath;
	std::string m_Ip;
	std::uint16_t m_Port = 0;

	std::optional<PrinterState> m_State = std::nullopt;

	std::unique_ptr<ShuiPrinterConnection> m_Connection;
	
	ShuiPrinterStorage m_Storage;
public:
	std::function<void()> OnStateChanged;
public:
	
	ShuiPrinter(std::string ip, std::uint16_t port, const std::filesystem::path &data_path);

	void RunAsync();

	virtual void IdentifyAsync();

	virtual void SetTargetBedTemperatureAsync(std::int64_t tempearture);

	virtual void SetTargetExtruderTemperatureAsync(std::int64_t tempearture);

	virtual void SetLCDMessageAsync(std::string message);

	virtual void SetDialogMessageAsync(std::string message, std::optional<int> display_time_seconds = std::nullopt);

	virtual void SetFanSpeedAsync(std::uint8_t speed);

	virtual void PauseUntillUserInputAsync(std::string message = "");

	virtual void PausePrintAsync();

	virtual void ResumePrintAsync();

	virtual void ReleaseMotorsAsync();

	virtual void CancelPrintAsync();
	
	virtual ShuiPrinterStorage &Storage();

	void OnConnectionConnect();

	void OnConnectionTick();

	void OnConnectionTimeout(std::int64_t timeout);

	void OnConnectionPrinterLine(const std::string &line, std::int64_t index);

	void SubmitReportSequenceAsync();

	void UpdateStateFromSystemLine(const std::string &line);

	void UpdateStateFromSdCardStatus(const std::string &line);

	void UpdateStateFromSelectedFile(const std::string &line);

	std::optional<PrinterState> GetPrinterState()const;

private:
	PrinterState &State();
};
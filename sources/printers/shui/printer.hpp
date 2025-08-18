#pragma once

#include "pch/std.hpp"
#include "printers/state.hpp"
#include "printers/shui/connection.hpp"
#include "printers/shui/storage.hpp"
#include "printers/shui/history.hpp"
#include "printers/printer.hpp"

namespace boost::asio { class io_context; }

class ShuiPrinter: public Printer {
	std::filesystem::path m_DataPath;
	std::string m_Ip;
	std::uint16_t m_Port = 0;

	std::optional<PrinterState> m_State = std::nullopt;

	std::unique_ptr<ShuiPrinterConnection> m_Connection;
	
	ShuiPrinterStorage m_Storage;
	ShuiPrinterHistory m_History;
public:
	
	ShuiPrinter(std::string ip, std::uint16_t port, const std::filesystem::path &data_path);

	void RunAsync()override;

	void HandleStateChanged();

	void IdentifyAsync(GCodeCallback callback)override;

	void SetTargetBedTemperatureAsync(std::int64_t tempearture, GCodeCallback callback)override;

	void SetTargetExtruderTemperatureAsync(std::int64_t tempearture, GCodeCallback callback)override;

	void SetFeedRatePercentAsync(float feed_rate, GCodeCallback callback)override;

	void SetLCDMessageAsync(std::string message, GCodeCallback callback)override;

	void SetDialogMessageAsync(std::string message, std::optional<int> display_time_seconds, GCodeCallback callback)override;

	void SetFanSpeedAsync(std::uint8_t speed, GCodeCallback callback)override;

	void PauseUntillUserInputAsync(std::string message, GCodeCallback callback)override;

	void PausePrintAsync(GCodeCallback callback)override;

	void ResumePrintAsync(GCodeCallback callback)override;

	void ReleaseMotorsAsync(GCodeCallback callback)override;

	void CancelPrintAsync(GCodeCallback callback)override;
	
	PrinterStorage &Storage()override;

	const PrinterHistory &History()const override;

	bool IsConnected()const override;

	void OnConnectionConnect();

	void OnConnectionTick();

	void OnConnectionTimeout(std::int64_t timeout);

	void OnConnectFailed(std::int64_t times);

	void OnConnectionLost();

	void OnConnectionPrinterLine(const std::string &line, std::int64_t index);

	void SubmitReportSequenceAsync();

	void UpdateStateFromSystemLine(const std::string &line);

	void UpdateStateFromSdCardStatus(const std::string &line);

	void UpdateStateFromSelectedFile(const std::string &line);

	void UpdateStateFromFeedRate(const std::string &line);

	std::optional<PrinterState> GetPrinterState()const override;

	bool TargetTemperaturesReached()const;

	bool AllHeatersOn()const;

private:
	PrinterState &State();
};
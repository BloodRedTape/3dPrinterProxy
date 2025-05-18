#pragma once

#include "pch/std.hpp"
#include "printers/state.hpp"
#include "printers/shui/connection.hpp"
#include "printers/shui/upload.hpp"

namespace boost::asio { class io_context; }

class ShuiPrinter {
	boost::asio::io_context& m_IoContext;
	std::string m_Ip;
	std::uint16_t m_Port = 0;

	std::optional<PrinterState> m_State = std::nullopt;

	std::unique_ptr<ShuiPrinterConnection> m_Connection;

public:
	std::function<void()> OnStateChanged;
public:
	
	ShuiPrinter(boost::asio::io_context &context, std::string ip, std::uint16_t port);

	void Run();

	virtual void Identify();

	virtual void SetTargetBedTemperature(std::int64_t tempearture);

	virtual void SetTargetExtruderTemperature(std::int64_t tempearture);

	virtual void SetLCDMessage(std::string message);

	virtual void SetFanSpeed(std::uint8_t speed);

	virtual void PauseUntillUserInput(std::string message = "");

	virtual void PausePrint();

	virtual void ResumePrint();

	virtual void ReleaseMotors();

	virtual void CancelPrint();

	virtual void UploadFile(const std::string &filename, const std::string &content, bool autostart = false);

	void OnConnectionConnect();

	void OnConnectionTick();

	void OnConnectionTimeout(std::int64_t timeout);

	void OnConnectionPrinterLine(const std::string &line, std::int64_t index);

	void SubmitReportSequence();

	void UpdateStateFromSystemLine(const std::string &line);

	void UpdateStateFromSdCardStatus(const std::string &line);

	void UpdateStateFromSelectedFile(const std::string &line);

	std::optional<PrinterState> GetPrinterState()const;

private:
	PrinterState &State();
};
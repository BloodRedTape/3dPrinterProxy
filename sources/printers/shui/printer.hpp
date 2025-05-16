#pragma once

#include <string>
#include <functional>
#include "printers/state.hpp"
#include "printers/shui/connection.hpp"

namespace boost::asio { class io_context; }

class ShuiPrinter {
	std::string m_Ip;
	std::uint16_t m_Port = 0;

	std::optional<PrinterState> m_State = std::nullopt;

	std::unique_ptr<ShuiPrinterConnection> m_Connection;

public:
	std::function<void()> OnStateChanged;
public:
	
	ShuiPrinter(std::string ip, std::uint16_t port);

	void RunStatePollingAsync(boost::asio::io_context &context);

	virtual void Identify();

	virtual void SetTargetBedTemperature(std::int64_t tempearture);

	virtual void SetTargetExtruderTemperature(std::int64_t tempearture);

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
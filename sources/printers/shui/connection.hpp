#pragma once

#include "printers/shui/gcode.hpp"
#include "pch/asio.hpp"

class ShuiPrinterConnection: public std::enable_shared_from_this<ShuiPrinterConnection> {
public:
	static constexpr char PrinterStreamLineSeparator = '\n';
private:
	boost::asio::ip::tcp::socket m_Socket;
	boost::asio::deadline_timer m_TimeoutTimer;

	std::string m_Ip;
	std::uint16_t m_Port = 0;
	std::int32_t m_SecondsTimeout = 0.f;
	
	char m_ReadBuffer[1024] = {};
	std::string m_TempBuffer;

	std::int64_t m_Timeouts = 0;
	std::int64_t m_Lines = 0;

	GCodeExecutionEngine m_GCodeEngine;
public:

	std::function<void(const std::string &, std::int64_t)> OnPrinterLine;

	std::function<void()> OnTick;
	std::function<void(std::int64_t)> OnTimeout;
	std::function<void()> OnConnect;
	
public:
	ShuiPrinterConnection(boost::asio::io_context &context, const std::string &ip, std::uint16_t port, std::int32_t seconds_timeout = 4);

	std::int64_t Timeouts()const;

	std::int32_t SecondsTimeout()const;

	void SubmitGCode(std::string gcode, GCodeSubmissionState::OnResultType on_result = [](auto){}, std::int64_t retries = 0);

	void CancelAllGCode();

	bool GCodeDone()const {
		return m_GCodeEngine.AllDone();
	}
	
	void Connect();

private:

	void HandleConnect(const boost::system::error_code& error);

	void Read();

	void HandleRead(const boost::system::error_code& error, size_t bytes_transferred);
		
	void HandlePrinterLine(const std::string &line);

	void StartReconnectTimeout();

	void CancelTimeout();

	void HandleTimeout(const boost::system::error_code& error);
};

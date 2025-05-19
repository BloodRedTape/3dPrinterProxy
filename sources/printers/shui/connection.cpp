#include "connection.hpp"
#include "core/async.hpp"
#include <bsl/log.hpp>

DEFINE_LOG_CATEGORY(ShuiConnection)

void LogShuiConnectionIf(const boost::system::error_code &ec) {
	LogShuiConnectionIf((bool)ec, Error, "%", ec.what());
}

ShuiPrinterConnection::ShuiPrinterConnection(const std::string &ip, std::uint16_t port, std::int32_t seconds_timeout):
	m_TimeoutTimer(Async::Context()),
	m_Socket(Async::Context()),
	m_Ip(ip),
	m_Port(port),
	m_SecondsTimeout(seconds_timeout)
{
	m_GCodeEngine.WriteGCode = [&](std::string gcode) {
		if(!gcode.size())
			return;

		if(gcode.back() != PrinterStreamLineSeparator)
			gcode.push_back(PrinterStreamLineSeparator);
		
		boost::system::error_code ec;
		m_Socket.write_some(boost::asio::buffer(gcode.data(), gcode.size()), ec);
		LogShuiConnectionIf(ec);
	};
}

std::int64_t ShuiPrinterConnection::Timeouts()const {
	return m_Timeouts;
}

std::int32_t ShuiPrinterConnection::SecondsTimeout()const {
	return m_SecondsTimeout;
}

void ShuiPrinterConnection::SubmitGCodeAsync(std::string gcode, GCodeSubmissionState::OnResultType on_result, std::int64_t retries) {
	m_GCodeEngine.Submit(std::move(gcode), on_result, retries);
}

void ShuiPrinterConnection::CancelAllGCode() {
	m_GCodeEngine.CancelAll();
}

void ShuiPrinterConnection::RunAsync() {
	Connect();
}

void ShuiPrinterConnection::Connect() {
	CancelTimeout();
	
	if (m_Socket.is_open()) {
		boost::system::error_code ec;
        m_Socket.cancel(ec);
		LogShuiConnectionIf(ec);
        m_Socket.close(ec);
		LogShuiConnectionIf(ec);
	}

	boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::make_address(m_Ip), m_Port);

	m_Socket.async_connect(endpoint, std::bind(&ShuiPrinterConnection::HandleConnect, this, std::placeholders::_1));

	StartReconnectTimeout();
}


void ShuiPrinterConnection::HandleConnect(const boost::system::error_code& error) {
	if(error == boost::system::errc::operation_canceled)
		return;

	if (error) {
		LogShuiConnection(Error, "OnConnect: %", error.what());
		Connect();
	} else {
		std::call(OnConnect);
		Read();
	}
}

void ShuiPrinterConnection::Read() {
	CancelTimeout();

	std::memset(m_ReadBuffer, 0, sizeof(m_ReadBuffer));

	m_Socket.async_read_some(boost::asio::buffer(m_ReadBuffer, sizeof(m_ReadBuffer) - 1), std::bind(&ShuiPrinterConnection::HandleRead, this, std::placeholders::_1, std::placeholders::_2));

	StartReconnectTimeout();
}

void ShuiPrinterConnection::HandleRead(const boost::system::error_code& error, size_t bytes_transferred) {
	if(error == boost::system::errc::operation_canceled)
		return;

	if (error) {
		LogShuiConnection(Error, "OnRead: %", error.what());
		return Connect();
	} 

	std::string transfered(m_ReadBuffer, std::strlen(m_ReadBuffer));

	m_TempBuffer += transfered;
	
	while (true) {
		auto separator_pos = m_TempBuffer.find(PrinterStreamLineSeparator);

		if(separator_pos == std::string::npos)
			break;
		
		std::string line = m_TempBuffer.substr(0, separator_pos);
		m_TempBuffer = m_TempBuffer.substr(separator_pos + 1);

		HandlePrinterLine(line);

		m_Lines++;
	}
	
	std::call(OnTick);

	Read();
	
	m_GCodeEngine.OnReadingDone(m_Lines);
}

void ShuiPrinterConnection::HandlePrinterLine(const std::string &line) {
#if SHUI_VERBOSE_LOGGING
	if(GCodeExecutionEngine::IsSystemLine(line))
		Println("[%][System]: %", m_Lines, line);
	else
		Println("[%]: %", m_Lines, line);
#endif

	std::call(OnPrinterLine, line, m_Lines);

	m_GCodeEngine.OnLine(line, m_Lines);
}

void ShuiPrinterConnection::StartReconnectTimeout() {
	boost::system::error_code ec;
    m_TimeoutTimer.expires_from_now(boost::posix_time::seconds(m_SecondsTimeout), ec);
	LogShuiConnectionIf(ec);
    
    m_TimeoutTimer.async_wait(std::bind(&ShuiPrinterConnection::HandleTimeout, this, std::placeholders::_1));
}

void ShuiPrinterConnection::CancelTimeout() {
	boost::system::error_code ec;
	m_TimeoutTimer.cancel(ec);
	LogShuiConnectionIf(ec);
}

void ShuiPrinterConnection::HandleTimeout(const boost::system::error_code& error) {
	if(error == boost::system::errc::operation_canceled){
		m_Timeouts = 0;
		return;
	}

    if (!error)
    {
		m_Lines = 0;
		m_Timeouts++;

		std::call(OnTimeout, m_Timeouts);

		Connect();
	}
}
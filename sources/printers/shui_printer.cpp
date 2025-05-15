#include "shui_printer.hpp"
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/algorithm/string.hpp>
#include <chrono>
#include <queue>
#include <bsl/log.hpp>

DEFINE_LOG_CATEGORY(Shui)

void LogShuiIf(const boost::system::error_code &ec) {
	LogShuiIf((bool)ec, Error, "%", ec.what());
}

enum class GCodeState {
	Enqueued,
	Sent	
};

struct GCodeSubmissionState {
	std::string GCode;
	std::function<void(const std::string &)> OnExecuted;
	GCodeState State = GCodeState::Enqueued;
	std::string Result;

	auto ToBuffer()const {
		return boost::asio::buffer(GCode.data(), GCode.size());
	}
};

class ShuiPrinterConnection: public std::enable_shared_from_this<ShuiPrinterConnection> {
	boost::asio::ip::tcp::socket m_Socket;
	boost::asio::deadline_timer m_TimeoutTimer;

	std::string m_Ip;
	std::uint16_t m_Port = 0;
	std::int32_t m_SecondsTimeout = 0.f;
	
	static constexpr char PrinterStreamLineSeparator = '\n';
	char m_ReadBuffer[1024] = {};
	std::string m_TempBuffer;

	std::int64_t m_Timeouts = 0;
	std::int64_t m_Lines = 0;

	std::queue<GCodeSubmissionState> m_SubmittedCommands;
public:
	std::function<void(const std::string &line)> OnPrinterLine;
public:
	ShuiPrinterConnection(boost::asio::io_context &context, const std::string &ip, std::uint16_t port, std::int32_t seconds_timeout = 4):
		m_TimeoutTimer(context),
		m_Socket(context),
		m_Ip(ip),
		m_Port(port),
		m_SecondsTimeout(seconds_timeout)
	{}

	std::int64_t Timeouts()const {
		return m_Timeouts;
	}

	std::int32_t SecondsTimeout()const {
		return m_SecondsTimeout;
	}

	void SubmitGCode(std::string gcode, std::function<void(const std::string &)> executed) {
		if(gcode.find(PrinterStreamLineSeparator) != gcode.size() - 1)
			gcode += '\n';
		
		m_SubmittedCommands.push({gcode, executed});
	}

	void Connect() {
		CancelTimeout();
		
		if (m_Socket.is_open()) {
			boost::system::error_code ec;
            m_Socket.cancel(ec);
			LogShuiIf(ec);
            m_Socket.close(ec);
			LogShuiIf(ec);
		}

		boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::make_address(m_Ip), m_Port);

		m_Socket.async_connect(endpoint, std::bind(&ShuiPrinterConnection::OnConnect, this, std::placeholders::_1));

		StartReconnectTimeout();
	}

	bool IsSystemLine(const std::string& line){
		return boost::starts_with(line, "ok T0:");
	}
private:


	void OnConnect(const boost::system::error_code& error) {
		if(error == boost::system::errc::operation_canceled)
			return;

		if (error) {
			LogShui(Error, "OnConnect: %", error.what());
			Connect();
		} else {
			Read();
		}
	}

	void Read() {
		CancelTimeout();

		std::memset(m_ReadBuffer, 0, sizeof(m_ReadBuffer));

		m_Socket.async_read_some(boost::asio::buffer(m_ReadBuffer, sizeof(m_ReadBuffer) - 1), std::bind(&ShuiPrinterConnection::OnRead, this, std::placeholders::_1, std::placeholders::_2));
	
		StartReconnectTimeout();
	}

	void OnRead(const boost::system::error_code& error, size_t bytes_transferred) {
		if(error == boost::system::errc::operation_canceled)
			return;

		if (error) {
			LogShui(Error, "OnRead: %", error.what());
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

			m_Lines++;

			HandlePrinterLine(line);
		}

		Read();

		if(!m_SubmittedCommands.size() || m_Lines <= 3)
			return;

		GCodeSubmissionState &current = m_SubmittedCommands.front();

		if(current.State == GCodeState::Enqueued){
			m_Socket.write_some(current.ToBuffer());
			current.State = GCodeState::Sent;
		}
	}

	void HandlePrinterLine(const std::string &line) {
		if(OnPrinterLine) OnPrinterLine(line);

		if(IsSystemLine(line))
			return;

		if(!m_SubmittedCommands.size())
			return;

		GCodeSubmissionState &current = m_SubmittedCommands.front();
		if (current.State == GCodeState::Sent) {
			if(boost::starts_with(line, "ok")){
				if(current.OnExecuted) current.OnExecuted(current.Result);

				m_SubmittedCommands.pop();
			} else {
				current.Result += line + "\n";
			}
		} 
	}

	void StartReconnectTimeout() {
		boost::system::error_code ec;
        m_TimeoutTimer.expires_from_now(boost::posix_time::seconds(m_SecondsTimeout), ec);
		LogShuiIf(ec);
        
        m_TimeoutTimer.async_wait(std::bind(&ShuiPrinterConnection::OnTimeout, this, std::placeholders::_1));
	}

	void CancelTimeout() {
		boost::system::error_code ec;
		m_TimeoutTimer.cancel(ec);
		LogShuiIf(ec);
	}

	void OnTimeout(const boost::system::error_code& error) {
		if(error == boost::system::errc::operation_canceled){
			m_Timeouts = 0;
			return;
		}

	    if (!error)
        {
			m_Lines = 0;
			m_Timeouts++;
			Connect();
		}
	}
};


ShuiPrinter::ShuiPrinter(std::string ip, std::uint16_t port):
	m_Ip(std::move(ip)),
	m_Port(port)
{}

void ShuiPrinter::RunStatePollingAsync(boost::asio::io_context & context){
	static auto sp = std::make_shared<ShuiPrinterConnection>(context, m_Ip, m_Port);
	sp->OnPrinterLine = [](auto line) {
		Println("Line: %", line);
	};
	sp->Connect();

	auto Print = [](const std::string& result) {
		Println("Result: %", result);
	};

	sp->SubmitGCode("M27", Print);
}

std::optional<PrinterState> ShuiPrinter::GetPrinterState() const {
	return m_State;
}

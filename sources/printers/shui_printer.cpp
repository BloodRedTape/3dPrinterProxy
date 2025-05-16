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
	using OnResultType = std::function<void(std::optional<std::string>)>;

	std::string GCode;
	OnResultType OnResult;
	std::int64_t Retries = 0;
	GCodeState State = GCodeState::Enqueued;
	std::string ResultAccumulator;
	std::int64_t SubmitedAfterLine = 0;
	
	bool CanRetry()const {
		return Retries > 0;
	}

	void MakeRetry() {
		assert(CanRetry());
		
		Retries--;
		State = GCodeState::Enqueued;
		ResultAccumulator = "";
		SubmitedAfterLine = 0;
	}
};


class GCodeExecutionEngine {
	std::queue<GCodeSubmissionState> m_SubmittedCommands;

	static constexpr std::int64_t PreambleLinesCount = 3;
public:
	std::function<void(const std::string &)> WriteGCode;
public:

	void SubmitGCode(std::string gcode, GCodeSubmissionState::OnResultType on_result, std::int64_t retries) {
		m_SubmittedCommands.push({gcode, on_result, retries});
	}

	static bool IsSystemLine(const std::string& line){
		return boost::starts_with(line, "ok T0:");
	}

	bool IsCommandFinishedLine(const std::string& line) {
		return !IsSystemLine(line) && boost::starts_with(line, "ok");
	}

	void OnReadingDone(std::int64_t last_index) {

		if(!m_SubmittedCommands.size() || last_index <= PreambleLinesCount)
			return;

		GCodeSubmissionState &current = m_SubmittedCommands.front();

		if(current.State == GCodeState::Enqueued){
			LogShuiIf(!WriteGCode, Error, "GCode writing callback is null");

			if(WriteGCode) WriteGCode(current.GCode);

			current.State = GCodeState::Sent;
			current.SubmitedAfterLine = last_index;
		}
	}

	void OnLine(const std::string& line, std::int64_t index) {
		if(!m_SubmittedCommands.size())
			return;

		GCodeSubmissionState &current = m_SubmittedCommands.front();
		if (current.State != GCodeState::Sent)
			return;

		//connection restarted
		if (index < current.SubmitedAfterLine) {
			if (current.CanRetry()) {
				current.MakeRetry();
			}else{
				if(current.OnResult) current.OnResult(std::nullopt);

				m_SubmittedCommands.pop();
			}
		}

		if(IsCommandFinishedLine(line)){
				//remove last \n
			if(current.ResultAccumulator.size())
				current.ResultAccumulator.pop_back();

			if(current.OnResult) current.OnResult(current.ResultAccumulator);
			LogShui(Display, "Command took % lines", index - current.SubmitedAfterLine);

			m_SubmittedCommands.pop();
		} else if(!IsSystemLine(line)) {
			current.ResultAccumulator += line + "\n";
		}
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

	GCodeExecutionEngine m_GCodeEngine;
public:

	std::function<void(const std::string &, std::int64_t)> OnPrinterLine;
public:
	ShuiPrinterConnection(boost::asio::io_context &context, const std::string &ip, std::uint16_t port, std::int32_t seconds_timeout = 4):
		m_TimeoutTimer(context),
		m_Socket(context),
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
			LogShuiIf(ec);
		};
	}

	std::int64_t Timeouts()const {
		return m_Timeouts;
	}

	std::int32_t SecondsTimeout()const {
		return m_SecondsTimeout;
	}

	void SubmitGCode(std::string gcode, GCodeSubmissionState::OnResultType on_result) {
		m_GCodeEngine.SubmitGCode(std::move(gcode), on_result, 1);
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

			HandlePrinterLine(line);

			m_Lines++;
		}

		Read();
		
		m_GCodeEngine.OnReadingDone(m_Lines);
	}

	void HandlePrinterLine(const std::string &line) {
		if(OnPrinterLine) OnPrinterLine(line, m_Lines);

		m_GCodeEngine.OnLine(line, m_Lines);
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
	sp->OnPrinterLine = [](auto line, auto line_number) {
		Println("[%]: %", line_number, line);
	};
	sp->Connect();

	static std::vector<std::string> results;
	static std::int64_t fails;

	auto Print = [&](std::optional<std::string> result) {
		if(result.has_value())
			results.push_back(result.value());
		else
			fails++;
	};

	sp->SubmitGCode("M27", Print);
	sp->SubmitGCode("M20", Print);
	sp->SubmitGCode("M20", Print);
	sp->SubmitGCode("M20", Print);
	sp->SubmitGCode("M20", Print);
	sp->SubmitGCode("M20", Print);
	sp->SubmitGCode("M20", Print);
	sp->SubmitGCode("M20", Print);
	sp->SubmitGCode("M20", Print);
	sp->SubmitGCode("M20", Print);
	sp->SubmitGCode("M20", Print);
	sp->SubmitGCode("M20", Print);
	sp->SubmitGCode("M20", Print);
	sp->SubmitGCode("M20", Print);
	sp->SubmitGCode("M20", Print);
	sp->SubmitGCode("M20", Print);
	sp->SubmitGCode("M27", [&](std::optional<std::string> result) {
		Println("CommandStats: Executed %, Failed %", results.size(), fails);
	});
}

std::optional<PrinterState> ShuiPrinter::GetPrinterState() const {
	return m_State;
}

#pragma once

#include <string>
#include <functional>
#include "printers/printer.hpp"

namespace boost::asio { class io_context; }

class ShuiPrinter {
	std::string m_Ip;
	std::uint16_t m_Port = 0;

	std::optional<PrinterState> m_State = std::nullopt;
public:
	std::function<void()> OnStateChanged;
public:
	
	ShuiPrinter(std::string ip, std::uint16_t port);

	void RunStatePollingAsync(boost::asio::io_context &context);

	std::optional<PrinterState> GetPrinterState()const;
};
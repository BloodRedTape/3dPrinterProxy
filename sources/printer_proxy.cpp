#include "printer_proxy.hpp"
#include "printers/shui_printer.hpp"


PrinterProxy::PrinterProxy(){
    m_Server.add_route("/api/v1/info")
		.get(std::bind(&PrinterProxy::GetInfo, this, std::placeholders::_1, std::placeholders::_2));
    m_Server.add_route("/api/v1/printers")
		.get(std::bind(&PrinterProxy::GetPrinters, this, std::placeholders::_1, std::placeholders::_2));
    m_Server.add_route("/api/v1/printers/:id")
		.get(std::bind(&PrinterProxy::GetPrinters, this, std::placeholders::_1, std::placeholders::_2));
}

void PrinterProxy::Listen(std::uint16_t port) {
	m_Server.listen(port);
}


int PrinterProxy::Run() {
	ShuiPrinter printer("192.168.1.179", 8080);

	printer.RunStatePollingAsync(m_IoContext);

	m_IoContext.run();
	return 0;
}

void PrinterProxy::GetInfo(const beauty::request& req, beauty::response& resp){
    resp.set(beauty::content_type::text_plain);
    resp.body() = "BloodRedTape's printer proxy";
}

void PrinterProxy::GetPrinters(const beauty::request& req, beauty::response& resp){
}

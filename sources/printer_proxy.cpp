#include "printer_proxy.hpp"
#include "printers/shui/printer.hpp"
#include <bsl/log.hpp>
#include <beauty/exception.hpp>

PrinterProxy::PrinterProxy(){
    m_Server.add_route("/api/v1/info")
		.get(std::bind(&PrinterProxy::GetInfo, this, std::placeholders::_1, std::placeholders::_2));
    m_Server.add_route("/api/v1/printers")
		.get(std::bind(&PrinterProxy::GetPrinters, this, std::placeholders::_1, std::placeholders::_2));
    m_Server.add_route("/api/v1/printers/:id/")
		.get(std::bind(&PrinterProxy::GetPrinters, this, std::placeholders::_1, std::placeholders::_2));

    m_Server.add_route("/api/v1/printers/:id/values/:type")
		.get(std::bind(&PrinterProxy::GetPrinterValue, this, std::placeholders::_1, std::placeholders::_2))
		.post(std::bind(&PrinterProxy::PostPrinterValue, this, std::placeholders::_1, std::placeholders::_2));
}

void PrinterProxy::Listen(std::uint16_t port) {
	m_Server.listen(port);
}

ShuiPrinter &GetPrinter(){
	static ShuiPrinter printer("192.168.1.179", 8080);
	return printer;
}

int PrinterProxy::Run() {

	GetPrinter().RunStatePollingAsync(m_IoContext);

	GetPrinter().OnStateChanged = []() {
		auto state_opt = GetPrinter().GetPrinterState();

		if (!state_opt.has_value()) {
			return Println("[Disconnected]");
		}

		auto state = state_opt.value();
		
		Print("[Bed %/%][Extruder %/%]", state.BedTemperature, state.TargetBedTemperature, state.ExtruderTemperature, state.TargetExtruderTemperature);
		
		if (state.Print.has_value()) {
			auto print = state.Print.value();
			Print("[filename %, progress %, status %, bytes %/%]", print.Filename, print.Progress, print.Status == PrintStatus::Busy ? "Busy" : "Printing", print.CurrentBytesPrinted, print.TargetBytesPrinted);
		}
		Print("\n");
	};

	m_IoContext.run();
	return 0;
}

void PrinterProxy::GetInfo(const beauty::request& req, beauty::response& resp){
    resp.set(beauty::content_type::text_plain);
    resp.body() = "BloodRedTape's printer proxy";
}

void PrinterProxy::GetPrinters(const beauty::request& req, beauty::response& resp){
}

void PrinterProxy::GetPrinterValue(const beauty::request& req, beauty::response& resp) {
	resp.body() = "Unimplemented";
	resp.set(beauty::content_type::text_plain);
}

void PrinterProxy::PostPrinterValue(const beauty::request& req, beauty::response& resp) {
	if (req.a("id").as_string() != "shui")
		throw beauty::http_error::client::bad_request();
	
	std::string type = req.a("type").as_string();

	if (type == "target_bed_temperature") {
		std::int64_t temp = std::stoi(req.body());

		GetPrinter().SetTargetBedTemperature(temp);
	}

	if (type == "target_extruder_temperature") {
		std::int64_t temp = std::stoi(req.body());

		GetPrinter().SetTargetExtruderTemperature(temp);
	}
}

void PrinterProxy::PostCommand(const beauty::request& req, beauty::response& resp) {

}

#include "printer_proxy.hpp"
#include "printers/shui/printer.hpp"
#include <bsl/log.hpp>

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

    m_Server.add_route("/api/v1/printers/:id/files/:filename")
		.post(std::bind(&PrinterProxy::PostStorageUpload, this, std::placeholders::_1, std::placeholders::_2));
}

void PrinterProxy::Listen(std::uint16_t port) {
	m_Server.listen(port);
}

void PrinterProxy::RunAsync() {
	m_Printer.RunAsync();

	m_Printer.OnStateChanged = [this]() {
		auto state_opt = m_Printer.GetPrinterState();

		if (!state_opt.has_value()) {
			return Println("[Disconnected]");
		}

		auto state = state_opt.value();
		
		Print("[Bed %/%][Extruder %/%]", state.BedTemperature, state.TargetBedTemperature, state.ExtruderTemperature, state.TargetExtruderTemperature);
		
		if (state.Print.has_value()) {
			auto print = state.Print.value();
			Print("[filename %, progress %, layer %, height %, status %, bytes %/%]", print.Filename, print.Progress, print.Layer, print.Height, to_string(print.Status), print.CurrentBytesPrinted, print.TargetBytesPrinted);
		}
		Print("\n");
	};
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

		m_Printer.SetTargetBedTemperatureAsync(temp);
	}

	if (type == "target_extruder_temperature") {
		std::int64_t temp = std::stoi(req.body());

		m_Printer.SetTargetExtruderTemperatureAsync(temp);
	}
}

void PrinterProxy::PostStorageUpload(const beauty::request& req, beauty::response& resp){
	if (req.a("id").as_string() != "shui")
		throw beauty::http_error::client::bad_request();
	
	std::string filename = req.a("filename").as_string();
	std::string content = req.body();

	if(!filename.size())
		throw beauty::http_error::client::bad_request();

	m_Printer.Storage().UploadGCodeFileAsync(filename, content, [](auto){});
}

#include "printer_proxy.hpp"
#include "printers/shui/printer.hpp"
#include "pch/std.hpp"
#include <bsl/log.hpp>
#include <bsl/file.hpp>
#include "config.hpp"

DEFINE_LOG_CATEGORY(Proxy);

PrinterProxy::PrinterProxy(){
    m_Server.add_route("/**")
		.get(std::bind(&PrinterProxy::GetFrontendFile, this, std::placeholders::_1, std::placeholders::_2));

    m_Server.add_route("/api/v1/info")
		.get(std::bind(&PrinterProxy::GetInfo, this, std::placeholders::_1, std::placeholders::_2));
    m_Server.add_route("/api/v1/printers")
		.get(std::bind(&PrinterProxy::GetPrinters, this, std::placeholders::_1, std::placeholders::_2));

    m_Server.add_route("/api/v1/printers/:id")
		.get(std::bind(&PrinterProxy::GetPrinter, this, std::placeholders::_1, std::placeholders::_2));

    m_Server.add_route("/api/v1/printers/:id/files/:filename/preview")
		.get(std::bind(&PrinterProxy::GetPreview, this, std::placeholders::_1, std::placeholders::_2));
    m_Server.add_route("/api/v1/printers/:id/files/:filename/metadata")
		.get(std::bind(&PrinterProxy::GetMetadata, this, std::placeholders::_1, std::placeholders::_2));

    m_Server.add_route("/api/v1/printers/:id/history")
		.get(std::bind(&PrinterProxy::GetHistory, this, std::placeholders::_1, std::placeholders::_2));

    m_Server.add_route("/api/v1/printers/:id/frontend")
		.get(std::bind(&PrinterProxy::GetFrontend, this, std::placeholders::_1, std::placeholders::_2));

	{
		auto id = "ttb_1";
		auto printer = std::make_shared<ShuiPrinter>("192.168.1.179", 8080, Format("./printers/%", id));

		m_Printers.emplace(id, printer);

		m_Interfaces.push_back(std::make_unique<OctoPrintInterface>(printer, 2229));
	}
	beauty::ws_handler handler;	
	handler.on_connect = std::bind(&PrinterProxy::WsOnConnect, this, std::placeholders::_1);
	handler.on_disconnect = std::bind(&PrinterProxy::WsOnDisconnect, this, std::placeholders::_1);
	handler.on_receive = std::bind(&PrinterProxy::WsOnReceive, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);
	handler.on_error = std::bind(&PrinterProxy::WsOnError, this, std::placeholders::_1, std::placeholders::_2);

	m_Server.add_route("/api/v1/ws").ws(std::move(handler));
}

void PrinterProxy::Listen(std::uint16_t port) {
	m_Server.listen(port);
}

void PrinterProxy::RunAsync() {

	for (auto& interface : m_Interfaces) {
		interface->RunAsync();
	}

	for (const auto& [id, printer] : m_Printers) {
		printer->RunAsync();

		printer->OnStateChanged = [this, printer, id]() {
			return BroadcastMessage(id, MessageType::state, StateToJson(printer->GetPrinterState()));
		};
	}

}

void PrinterProxy::GetFrontendFile(const beauty::request& req, beauty::response& resp) {
    std::string_view full_target = req.target();

	std::string_view path = full_target;
	size_t pos = path.find('?');
	if (pos != std::string_view::npos) {
		path = path.substr(0, pos);
	}

	auto file = std::remove_all_front(path, '/');

	if(!file.size())
		file = "index.html";

	std::filesystem::path filepath = std::filesystem::path(Config::FrontentPath) / file;

	if(!std::filesystem::exists(filepath))
		throw beauty::http_error::client::not_found();

	beauty::header::content_type content_type("application/octet-stream");

	auto ext = filepath.extension().string();

	if (beauty::content_type::types.count(ext)) {
		content_type = beauty::content_type::types.at(ext);
	}
	
	resp.body() = File::ReadEntire(filepath);
	resp.set(content_type);
}

void PrinterProxy::GetInfo(const beauty::request& req, beauty::response& resp){
    resp.set(beauty::content_type::text_plain);
    resp.body() = "BloodRedTape's printer proxy";
}

void PrinterProxy::GetPrinters(const beauty::request& req, beauty::response& resp) {
	resp.set(beauty::content_type::application_json);
	resp.body() = nlohmann::json(PrintersIds()).dump();
}

void PrinterProxy::GetPrinter(const beauty::request& req, beauty::response& resp) {
	auto id = req.a("id").as_string();

	if(!m_Printers.count(id))
		throw beauty::http_error::client::not_found();
	
	auto printer = m_Printers.at(id);

	resp.set(beauty::content_type::application_json);
	resp.body() = nlohmann::json::object({
		{"model", printer->Model},
		{"manufacturer", printer->Manufacturer},
	}).dump();
}

void PrinterProxy::GetPreview(const beauty::request& req, beauty::response& resp) {
	auto id = req.a("id").as_string();
	auto filename = req.a("filename").as_string();

	if(!m_Printers.count(id))
		throw beauty::http_error::client::not_found();
	
	auto printer = m_Printers.at(id);
	const GCodeFileMetadata *metadata = printer->Storage().GetMetadata(filename);

	if(!metadata)
		throw beauty::http_error::client::not_found();
	
	if(!metadata->Previews.size())
		throw beauty::http_error::client::not_found();

	resp.body() = metadata->Previews.back().ToPng();
	resp.set(beauty::content_type::image_png);
}

void PrinterProxy::GetMetadata(const beauty::request& req, beauty::response& resp) {
	auto id = req.a("id").as_string();
	auto filename = req.a("filename").as_string();

	if(!m_Printers.count(id))
		throw beauty::http_error::client::not_found();
	
	auto printer = m_Printers.at(id);
	const GCodeFileMetadata *metadata = printer->Storage().GetMetadata(filename);

	if(!metadata)
		throw beauty::http_error::client::not_found();

	resp.body() = nlohmann::json(*metadata).dump();
	resp.set(beauty::content_type::application_json);
}

void PrinterProxy::GetHistory(const beauty::request& req, beauty::response& resp){
	auto id = req.a("id").as_string();

	if(!m_Printers.count(id))
		throw beauty::http_error::client::not_found();
	
	auto printer = m_Printers.at(id);
	auto history = printer->History().GetHistory();

	resp.body() = nlohmann::json(history).dump();
	resp.set(beauty::content_type::application_json);
}

void PrinterProxy::GetFrontend(const beauty::request& req, beauty::response& resp) {
	std::string ui_host = "http://127.0.0.1:2228";

	resp.result(beauty::http::status::found);
    resp.set(beauty::http::field::server, "Boost.Beast Redirect Example");
    resp.set(beauty::http::field::location, ui_host);
}

void PrinterProxy::OnSet(const std::string& id, const nlohmann::json& content) {
	if(!m_Printers.count(id))
		return LogProxy(Error, "Unknwon printer id %", id);
	
	auto printer = m_Printers[id];

	try{
		MessageSet set = content;
		if (set.property == "target_bed") {
			printer->SetTargetBedTemperatureAsync(set.value);
		}
		if (set.property == "target_extruder") {
			printer->SetTargetExtruderTemperatureAsync(set.value);
		}
		if (set.property == "feedrate") {
			printer->SetFeedRatePercentAsync(set.value);
		}
	}catch (const std::exception &e) {
		LogProxy(Error, "OnSet: %", e.what());
	}
}

void PrinterProxy::WsOnConnect(const beauty::ws_context& ctx) {
	m_Sessions[ctx.uuid] = ctx.ws_session;
	
	for (const auto& [id, printer] : m_Printers) {
		SendMessage(ctx.ws_session, id, MessageType::init, nullptr);

		if(printer->IsConnected())
			SendMessage(ctx.ws_session, id, MessageType::state, StateToJson(printer->GetPrinterState()));
	}
}

void PrinterProxy::WsOnReceive(const beauty::ws_context& ctx, const char* data, std::size_t size, bool is_text) {
	try{
		Message message = nlohmann::json::parse(std::string_view(data, size), nullptr, false, false);
		
		if (message.type == "set") {
			OnSet(message.id, message.content);
		}else {
			LogProxy(Warning, "Unsupported message type % for printer %", message.type, message.id);
		}
	}catch (const std::exception &e) {
		LogProxy(Error, "Receive failed: %", e.what());
	}
}

void PrinterProxy::WsOnDisconnect(const beauty::ws_context& ctx) {
	m_Sessions.erase(ctx.uuid);
}

void PrinterProxy::SendMessage(std::weak_ptr<beauty::websocket_session> session, const std::string &id, MessageType type, const nlohmann::json& content) {
	if (auto ptr = session.lock()) {
		Message message;
		message.id = id;
		message.type = type.Name();
		message.content = content;
		ptr->send(message.ToJson());
	}
}

void PrinterProxy::BroadcastMessage(const std::string &id, MessageType type, const nlohmann::json& content) {
	for (const auto& [uuid, session]: m_Sessions) {
		SendMessage(session, id, type, content);
	}
}

void PrinterProxy::WsOnError(boost::system::error_code ec, const char* what) {
	LogProxy(Error, "%: %", ec.message(), what);
}

nlohmann::json PrinterProxy::StateToJson(const std::optional<PrinterState>& state_opt) {
	nlohmann::json state_json;

	if (!state_opt.has_value()) {
		return state_json;
	}

	auto state = state_opt.value();
	
	state_json["bed"] = state.BedTemperature;
	state_json["target_bed"] = state.TargetBedTemperature;
	state_json["extruder"] = state.ExtruderTemperature;
	state_json["target_extruder"] = state.TargetExtruderTemperature;
	state_json["feedrate"] = state.FeedRate;
	state_json["print"] = nullptr;
	
	if (!state.Print.has_value()) {
		return state_json;
	}

	auto print = state.Print.value();

	state_json["print"] = nlohmann::json::object();
	state_json["print"]["filename"] = print.Filename;
	state_json["print"]["progress"] = print.Progress;
	state_json["print"]["layer"] = print.Layer;
	state_json["print"]["height"] = print.Height;
	state_json["print"]["status"] = print.Status.Name();
	state_json["print"]["bytes_current"] = print.CurrentBytesPrinted;
	state_json["print"]["bytes_target"] = print.TargetBytesPrinted;

	return state_json;
}

std::vector<std::string> PrinterProxy::PrintersIds()const {
	std::vector<std::string> result;

	for (const auto& [id, _] : m_Printers) {
		result.push_back(id);
	}

	return result;
}

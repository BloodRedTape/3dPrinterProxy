#include "printer_proxy.hpp"


PrinterProxy::PrinterProxy(){
    m_Server.add_route("/info")
        .get([this](const beauty::request& req, beauty::response& res) {
            res.set(beauty::content_type::text_plain);
            res.body() = "3d Printer Proxy Server";
        });
}

void PrinterProxy::Listen(std::uint16_t port) {
	m_Server.listen(port);
}

int PrinterProxy::Run() {
	m_IoContext.run();
	return 0;
}

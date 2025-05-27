#pragma once

#include "pch/beauty.hpp"
#include "core/async.hpp"
#include "printers/printer.hpp"

class OctoPrintInterface {
private:
    beauty::application m_BeautyApplication{Async::Context()};
    beauty::server m_Server{m_BeautyApplication};
    
    Printer* m_Printer = nullptr;
public:
    OctoPrintInterface(Printer *printer);

    void Listen(std::uint16_t port);
	
    void RunAsync();

    void GetVersion(const beauty::request &req, beauty::response &resp);

    void PostFilesLocal(const beauty::request &req, beauty::response &resp);
};

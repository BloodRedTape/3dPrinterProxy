#pragma once

#include "pch/beauty.hpp"
#include "core/async.hpp"
#include "printers/printer.hpp"

class OctoPrintInterface {
private:
    beauty::application m_BeautyApplication{Async::Context()};
    beauty::server m_Server{m_BeautyApplication};
    
    std::shared_ptr<Printer> m_Printer;
public:
    OctoPrintInterface(std::shared_ptr<Printer> printer, std::uint16_t port);
	
    void RunAsync();

    void GetVersion(const beauty::request &req, beauty::response &resp);

    void PostFilesLocal(const beauty::request &req, beauty::response &resp);
};

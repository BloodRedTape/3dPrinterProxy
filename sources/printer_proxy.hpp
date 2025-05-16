#pragma once

#include <beauty/server.hpp>

class PrinterProxy {
private:
    boost::asio::io_context m_IoContext;
    beauty::application m_BeautyApplication{m_IoContext};
    beauty::server m_Server{m_BeautyApplication};
public:
    PrinterProxy();

    void Listen(std::uint16_t port);

    int Run();

    void GetInfo(const beauty::request &req, beauty::response &resp);

    void GetPrinters(const beauty::request &req, beauty::response &resp);

    void GetPrinterValue(const beauty::request &req, beauty::response &resp);

    void PostPrinterValue(const beauty::request &req, beauty::response &resp);

    void PostCommand(const beauty::request &req, beauty::response &resp);
};

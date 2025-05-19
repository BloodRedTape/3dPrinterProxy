#pragma once

#include "pch/beauty.hpp"
#include "printers/shui/printer.hpp"
#include "core/async.hpp"

class PrinterProxy {
private:
    beauty::application m_BeautyApplication{Async::Context()};
    beauty::server m_Server{m_BeautyApplication};

    ShuiPrinter m_Printer{"192.168.1.179", 8080, "./printers/twotreesbluer"};
public:
    PrinterProxy();

    void Listen(std::uint16_t port);

    void RunAsync();

    void GetInfo(const beauty::request &req, beauty::response &resp);

    void GetPrinters(const beauty::request &req, beauty::response &resp);

    void GetPrinterValue(const beauty::request &req, beauty::response &resp);

    void PostPrinterValue(const beauty::request &req, beauty::response &resp);

    void PostStorageUpload(const beauty::request &req, beauty::response &resp);
};

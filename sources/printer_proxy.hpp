#pragma once

#include <beauty/beauty.hpp>

class PrinterProxy {
private:
    boost::asio::io_context m_IoContext;
    beauty::application m_BeautyApplication{m_IoContext};
    beauty::server m_Server{m_BeautyApplication};
public:
    PrinterProxy();

    void Listen(std::uint16_t port);

    int Run();
};

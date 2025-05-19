#pragma once

#include "pch/std.hpp"
#include "pch/asio.hpp"

class ShuiUpload: public std::enable_shared_from_this<ShuiUpload> {
public:
    using CompletionCallback = std::function<void(bool success, const std::string& message)>;

private:
    boost::asio::ip::tcp::socket m_Socket;
    std::string m_Ip;
    std::string m_Filename;
    std::string m_Content;
    bool m_StartPrinting;
    std::string m_Boundary;
    CompletionCallback m_Callback;
    boost::beast::http::request<boost::beast::http::string_body> m_Request;
    boost::beast::flat_buffer m_Buffer;
    boost::beast::http::response<boost::beast::http::string_body> m_Response;

public:
    ShuiUpload(const std::string& ip, const std::string& filename, const std::string& content, bool start_printing = false, CompletionCallback callback = nullptr);
    
    void RunAsync();

private:
    std::string GenerateBoundary();

    boost::beast::http::request<boost::beast::http::string_body> CreateMultipartRequest();

    void Connect();

    void OnConnect(boost::beast::error_code ec);

    void OnWrite(boost::beast::error_code ec, std::size_t bytes_transferred);

    void OnRead(boost::beast::error_code ec, std::size_t bytes_transferred);
};
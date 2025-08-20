#pragma once

#include "pch/std.hpp"
#include "pch/asio.hpp"

class ShuiUpload: public std::enable_shared_from_this<ShuiUpload> {
public:
    using CompletionCallback = std::function<void(std::variant<std::string, const std::string*>)>;
    using ProgressCallback = std::function<void(std::int64_t, std::int64_t)>;

private:
    boost::asio::ip::tcp::socket m_Socket;
    std::string m_Ip;
    std::string m_Filename;
    std::string m_Content;
    bool m_StartPrinting;
    std::string m_Boundary;
    CompletionCallback m_Callback;
    ProgressCallback m_Progress;
    boost::beast::http::request<boost::beast::http::string_body> m_Request;
    std::string m_SerializedRequest;
    std::size_t m_BytesWritten = 0;
    boost::beast::flat_buffer m_Buffer;
    boost::beast::http::response<boost::beast::http::string_body> m_Response;

public:
    ShuiUpload(boost::asio::io_context& context, const std::string& ip, const std::string& filename, std::string&& content, bool start_printing = false, CompletionCallback callback = nullptr, ProgressCallback progress = nullptr);
    
    static void RunAsync(const std::string& ip, const std::string& filename, std::string&& content, bool start_printing = false, CompletionCallback callback = nullptr, ProgressCallback progress = nullptr);

    static std::optional<std::string> Run(const std::string& ip, const std::string& filename, const std::string& content, bool start_printing = false, ProgressCallback progress = nullptr);
private:
    std::string GenerateBoundary();

    boost::beast::http::request<boost::beast::http::string_body> CreateMultipartRequest();

    void Connect();

    void OnConnect(boost::beast::error_code ec);

    void WriteNextChunk();

    void OnWrite(boost::beast::error_code ec, std::size_t bytes_transferred);

    void OnRead(boost::beast::error_code ec, std::size_t bytes_transferred);
};
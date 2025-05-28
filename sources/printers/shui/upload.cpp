#include "upload.hpp"
#include "core/async.hpp"
#include <random>
#include <sstream>
#include <iomanip>
#include <bsl/log.hpp>

ShuiUpload::ShuiUpload(const std::string& ip, const std::string& filename, std::string&& content, bool start_printing, CompletionCallback callback): 
    m_Socket(Async::Context()), 
    m_Ip(ip), 
    m_Filename(filename), 
    m_Content(std::move(content)), 
    m_StartPrinting(start_printing), 
    m_Callback(callback) 
{
    m_Boundary = GenerateBoundary();
}

void ShuiUpload::RunAsync() {
    Connect();
}

std::string ShuiUpload::GenerateBoundary() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);
    
    std::stringstream ss;
    ss << "------------------------";
    for (int i = 0; i < 24; i++) {
        ss << std::hex << dis(gen);
    }
    return ss.str();
}

boost::beast::http::request<boost::beast::http::string_body> ShuiUpload::CreateMultipartRequest() {
    boost::beast::http::request<boost::beast::http::string_body> req{boost::beast::http::verb::post, "/upload", 11};
    req.set(boost::beast::http::field::host, m_Ip);
    req.set(boost::beast::http::field::content_type, "multipart/form-data; boundary=" + m_Boundary);
    
    if (m_StartPrinting) {
        req.set("Start-Printing", "1");
    }
    
    std::string body;
    body += "--" + m_Boundary + "\r\n";
    body += "Content-Disposition: form-data; name=\"file\"; filename=\"" + m_Filename + "\"\r\n";
    body += "\r\n";
    body += m_Content;
    body += "\r\n--" + m_Boundary + "--\r\n";
    
    req.body() = body;
    req.prepare_payload();
    
    return req;
}

void ShuiUpload::Connect() {
    boost::beast::error_code ec;
    boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::make_address(m_Ip, ec), 80);
    if(ec) {
        if (m_Callback) {
            m_Callback(false, "Invalid IP address: " + ec.message());
        }
        return;
    }
    
    m_Socket.async_connect(endpoint, boost::beast::bind_front_handler(&ShuiUpload::OnConnect, shared_from_this()));
}

void ShuiUpload::OnConnect(boost::beast::error_code ec) {
    if(ec) {
        if (m_Callback) {
            m_Callback(false, "Connect failed: " + ec.message());
        }
        return;
    }
    
    m_Request = CreateMultipartRequest();
    
    boost::beast::http::async_write(m_Socket, m_Request, boost::beast::bind_front_handler(&ShuiUpload::OnWrite, shared_from_this()));
}

void ShuiUpload::OnWrite(boost::beast::error_code ec, std::size_t bytes_transferred) {
    if(ec) {
        if (m_Callback) {
            m_Callback(false, "Write failed: " + ec.message());
        }
        return;
    }
    
    boost::beast::http::async_read(m_Socket, m_Buffer, m_Response, boost::beast::bind_front_handler(&ShuiUpload::OnRead, shared_from_this()));
}

void ShuiUpload::OnRead(boost::beast::error_code ec, std::size_t bytes_transferred) {
    if(ec && ec != boost::beast::errc::not_connected) {
        if (m_Callback) {
            m_Callback(false, "Read failed: " + ec.message());
        }
        return;
    }
    
    bool success = m_Response.result() == boost::beast::http::status::ok || 
                  m_Response.result() == boost::beast::http::status::created || 
                  m_Response.result() == boost::beast::http::status::accepted;

    if (m_Callback) {
        if (success) {
            m_Callback(true, "File uploaded successfully");
        } else {
            m_Callback(false, "Server returned error code: " + 
                      std::to_string(static_cast<int>(m_Response.result())));
        }
    }
    
    boost::beast::error_code close_ec;
    m_Socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, close_ec);
}
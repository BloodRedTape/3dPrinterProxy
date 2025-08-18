#include "upload.hpp"
#include "core/async.hpp"
#include <random>
#include <sstream>
#include <iomanip>
#include <bsl/log.hpp>

ShuiUpload::ShuiUpload(boost::asio::io_context &context, const std::string& ip, const std::string& filename, std::string&& content, bool start_printing, CompletionCallback callback): 
    m_Socket(context), 
    m_Ip(ip), 
    m_Filename(filename), 
    m_Content(std::move(content)), 
    m_StartPrinting(start_printing), 
    m_Callback(callback) 
{
    m_Boundary = GenerateBoundary();
}


void ShuiUpload::RunAsync(const std::string& ip, const std::string& filename, std::string&& content, bool start_printing, CompletionCallback callback) {
    std::make_shared<ShuiUpload>(Async::Context(), ip, filename, std::move(content), start_printing, callback)->Connect();
}

std::optional<std::string> ShuiUpload::Run(const std::string& ip, const std::string& filename, const std::string& content, bool start_printing) {
    boost::asio::io_context blocking_context;

    std::optional<std::variant<std::string, const std::string*>> result_opt;

    auto upload = std::make_shared<ShuiUpload>(blocking_context, ip, filename, std::string(content), start_printing, [&](std::variant<std::string, const std::string*> got_result) {
        result_opt = std::move(got_result);
    });

    upload->Connect();
    
    //Limit file upload to one minute to fix freezes on error
    static constexpr const auto FileUploadMaxTimeLimit = std::chrono::seconds(120);
    blocking_context.run_for(FileUploadMaxTimeLimit);

    if(!result_opt.has_value())
        return "Hard Timeout";

    const auto &result = result_opt.value();

    return result.index() == 0 ? std::optional<std::string>(std::get<0>(result)) : std::nullopt;
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
            m_Callback("Invalid IP address: " + ec.message());
        }
        return;
    }
    
    m_Socket.async_connect(endpoint, boost::beast::bind_front_handler(&ShuiUpload::OnConnect, shared_from_this()));
}

void ShuiUpload::OnConnect(boost::beast::error_code ec) {
    if(ec) {
        if (m_Callback) {
            m_Callback("Connect failed: " + ec.message());
        }
        return;
    }
    
    m_Request = CreateMultipartRequest();

    std::ostringstream ss;
    ss << m_Request;
    m_SerializedRequest = ss.str();
    
    m_BytesWritten = 0;
    WriteNextChunk();    
}

void ShuiUpload::WriteNextChunk() {
    static constexpr std::size_t CHUNK_SIZE = 8 * 1024; // 8KB
    
    if (m_BytesWritten >= m_SerializedRequest.size()) {
        OnWrite(boost::beast::error_code{}, m_BytesWritten);
        return;
    }
    
    std::size_t remaining = m_SerializedRequest.size() - m_BytesWritten;
    std::size_t to_write = std::min(CHUNK_SIZE, remaining);
    
    auto buffer = boost::asio::buffer(m_SerializedRequest.data() + m_BytesWritten, to_write);
    
    boost::asio::async_write(m_Socket, buffer,
        [this, self = shared_from_this()](boost::beast::error_code ec, std::size_t bytes) {
            if (ec) {
                OnWrite(ec, m_BytesWritten);
                return;
            }
            
            m_BytesWritten += bytes;
            WriteNextChunk();
        });
}

void ShuiUpload::OnWrite(boost::beast::error_code ec, std::size_t bytes_transferred) {
    if(ec) {
        if (m_Callback) {
            m_Callback("Write failed: " + ec.message());
        }
        return;
    }
    
    boost::beast::http::async_read(m_Socket, m_Buffer, m_Response, boost::beast::bind_front_handler(&ShuiUpload::OnRead, shared_from_this()));
}

void ShuiUpload::OnRead(boost::beast::error_code ec, std::size_t bytes_transferred) {
    if(ec && ec != boost::beast::errc::not_connected) {
        if (m_Callback) {
            m_Callback("Read failed: " + ec.message());
        }
        return;
    }
    
    bool success = m_Response.result() == boost::beast::http::status::ok || 
                  m_Response.result() == boost::beast::http::status::created || 
                  m_Response.result() == boost::beast::http::status::accepted;

    if (m_Callback) {
        if (success) {
            m_Callback(&m_Content);
        } else {
            m_Callback("Server returned error code: " + 
                      std::to_string(static_cast<int>(m_Response.result())));
        }
    }
    
    boost::beast::error_code close_ec;
    m_Socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, close_ec);
}
#include "octo_print.hpp"
#include <bsl/log.hpp>
#include "pch/asio.hpp"
#include "core/string_utils.hpp"

OctoPrintInterface::OctoPrintInterface(std::shared_ptr<Printer> printer, std::uint16_t port):
	m_Printer(printer)
{
	m_Server.add_route("/api/version")
		.get(std::bind(&OctoPrintInterface::GetVersion, this, std::placeholders::_1, std::placeholders::_2));
	m_Server.add_route("/api/files/local")
		.post(std::bind(&OctoPrintInterface::PostFilesLocal, this, std::placeholders::_1, std::placeholders::_2));

    m_Server.listen(port);
}

void OctoPrintInterface::RunAsync() {
	//Idk
}

void OctoPrintInterface::GetVersion(const beauty::request& req, beauty::response& resp) {
	resp.body() = R"({
		"api": "0.1",
		"server": "1.3.10",
		"text": "OctoPrint 1.3.10"
	})";
	resp.set(beauty::content_type::application_json);
}

struct MultipartPart {
    std::map<std::string, std::string> headers;
    std::string_view content;
};

static std::string_view ExtractBoundary(std::string_view contentType) {
    size_t boundaryPos = contentType.find("boundary=");
    if (boundaryPos == std::string_view::npos) {
        return {};
    }
    
    std::string_view boundary = contentType.substr(boundaryPos + 9);
    if (!boundary.empty() && boundary.front() == '"') {
        boundary.remove_prefix(1);
    }
    if (!boundary.empty() && boundary.back() == '"') {
        boundary.remove_suffix(1);
    }
    return boundary;
}

static bool ParseMultipartFrame(std::string_view body, std::size_t start, std::string_view boundary, std::string_view &part, std::size_t &part_end) {
    auto boundary1 = body.find(boundary, start);

    if(boundary1 == std::string_view::npos)
        return false;

    auto boundary2 = body.find(boundary, boundary1 + boundary.size());

    if(boundary2 == std::string_view::npos)
        return false;
    
    part = body.substr(boundary1 + boundary.size(), boundary2 - boundary1 - boundary.size());
    part_end = boundary2;
    return true;
}

static std::string_view ParseMultipartContent(std::string_view frame) {
    static constexpr std::string_view ContentPrefix = "\r\n\r\n";
    static constexpr std::string_view ContentSuffix = "\r\n";
    return SubstrBy(frame, ContentPrefix, ContentSuffix);
}

std::map<std::string, std::string> ParseMultipartHeaders(std::string_view frame) {
    static constexpr std::string_view HeadersPrefix = "Content-Disposition: form-data;";
    static constexpr std::string_view HeadersSuffix = "\n";
    
    auto line = SubstrBy(frame, HeadersPrefix, HeadersSuffix);

    if(!line.size())
        return {};

    std::map<std::string, std::string> result;

    for (;;) {
        std::string_view name = SubstrBy(line, " ", "=");
        std::string_view value = SubstrBy(line, "\"", "\"");

        if (!name.size() || !value.size())
            break;

        line = line.substr(name.size() + value.size() + 4);

        result.emplace(name, value);
    }

    return result;
}

static std::vector<MultipartPart> ParseMultipartData(std::string_view body, std::string_view boundaryParam) {
    std::vector<MultipartPart> parts;
    
    std::string delimiter = std::string("--") + std::string(boundaryParam);
    
    
    std::size_t pos = 0;
    std::size_t end_pos = 0;
    std::string_view part;

    while(ParseMultipartFrame(body, pos, delimiter, part, end_pos)) {
        pos = end_pos;


        MultipartPart multipart;
        multipart.content = ParseMultipartContent(part);
        multipart.headers = ParseMultipartHeaders(part);
        parts.push_back(multipart);
    }
    
        
    return parts;
}

void OctoPrintInterface::PostFilesLocal(const beauty::request& req, beauty::response& resp) {
#if !WITH_PRINTER_DEBUG
    if(!m_Printer || !m_Printer->IsConnected()) {
        throw beauty::http_error::server::service_unavailable();
    }
#endif

    std::string_view contentType = req.base().at("Content-Type");
    std::string_view boundary = ExtractBoundary(contentType);
    
    if (boundary.empty()) {
        throw beauty::http_error::client::bad_request("Invalid multipart boundary");
    }
    
    std::vector<MultipartPart> parts = ParseMultipartData(req.body(), boundary);

    std::string_view print;
    std::string_view file_content;
    std::string filename;
    
    for (const auto& part : parts) {
        auto nameIt = part.headers.find("name");

        if (nameIt == part.headers.end()) {
            continue;
        }
        
        if (nameIt->second == "print") {
            print = part.content;
        } 
        if (nameIt->second == "file") {
            file_content = part.content;
            auto filenameIt = part.headers.find("filename");
            if (filenameIt != part.headers.end()) {
                filename = filenameIt->second;
            }
        }
    }
    
    bool should_print = (print == "true");
    bool uploaded = m_Printer->Storage().UploadGCodeFile(filename, std::string(file_content), should_print);

    if(!uploaded)
        throw beauty::http_error::client::failed_dependency();
    
    resp.body() = R"({"done": true})";
    resp.set(beauty::content_type::application_json);
}
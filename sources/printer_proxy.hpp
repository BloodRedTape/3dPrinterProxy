#pragma once

#include "pch/beauty.hpp"
#include "printers/shui/printer.hpp"
#include "interfaces/octo_print.hpp"
#include "core/async.hpp"
#include <bsl/enum.hpp>

#ifdef SendMessage
#undef SendMessage
#endif


BSL_ENUM(MessageType,
    init,
    state
);

struct Message {
    std::string type;
    std::string id;
    nlohmann::json content;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(Message, type, id, content);

    std::string ToJson()const {
        return nlohmann::json(*this).dump();
    }
};

struct MessageSet {
    std::string property;
    nlohmann::json value;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(MessageSet, property, value);
};

class PrinterProxy {
private:
    beauty::application m_BeautyApplication{Async::Context()};
    beauty::server m_Server{m_BeautyApplication};
    
    std::map<std::string, std::shared_ptr<Printer>> m_Printers;
    std::vector<std::unique_ptr<OctoPrintInterface>> m_Interfaces;

    std::map<std::string, std::weak_ptr<beauty::websocket_session>> m_Sessions;
public:
    PrinterProxy();

    void Listen(std::uint16_t port);

    void RunAsync();

    void GetFrontendFile(const beauty::request &req, beauty::response &resp);

    void GetInfo(const beauty::request &req, beauty::response &resp);

    void GetPrinters(const beauty::request &req, beauty::response &resp);

    void GetPrinter(const beauty::request &req, beauty::response &resp);

    void GetPreview(const beauty::request &req, beauty::response &resp);

    void GetMetadata(const beauty::request &req, beauty::response &resp);

    void GetHistory(const beauty::request &req, beauty::response &resp);

    void GetFrontend(const beauty::request &req, beauty::response &resp);

    void OnSet(const std::string &id, const nlohmann::json& content);

    void WsOnConnect(const beauty::ws_context& ctx);
    void WsOnReceive(const beauty::ws_context& ctx, const char*, std::size_t, bool);
    void WsOnDisconnect(const beauty::ws_context& ctx);
    void WsOnError(boost::system::error_code, const char* what);

    void SendMessage(std::weak_ptr<beauty::websocket_session> session, const std::string &id, MessageType type, const nlohmann::json &content);
    void BroadcastMessage(const std::string &id, MessageType type, const nlohmann::json &content);

    static nlohmann::json StateToJson(const std::optional<PrinterState> &state);

    std::vector<std::string> PrintersIds()const;

};

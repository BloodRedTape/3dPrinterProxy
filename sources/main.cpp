#include <filesystem>
#include <bsl/log.hpp>
#include <chrono>
#include "printer_proxy.hpp"
#include "simple/tg_logger.hpp"
#include "config.hpp"

std::unique_ptr<SimpleTgLogger> s_Logger;

void LogFunctionExternal(const std::string& category, Verbosity verbosity, const std::string& message) {
	if(!s_Logger)
		return;

	auto line = Format("[%][%]: %", category, verbosity, message);

	Println("%", line);

	if(verbosity <= Verbosity::Display)
		return;
	
	s_Logger->Log(line);
}

void OnNewDay(std::function<void()> func){
    auto now = std::chrono::system_clock::now();
    time_t tt = std::chrono::system_clock::to_time_t(now);
    std::tm local_tm = *std::localtime(&tt);

    // Set to next 12:00 PM
    local_tm.tm_hour = 12;
    local_tm.tm_min = 0;
    local_tm.tm_sec = 0;
    auto target_time = std::chrono::system_clock::from_time_t(std::mktime(&local_tm));
    if (now >= target_time)
        target_time += std::chrono::hours(24);

    auto wait_duration = duration_cast<std::chrono::steady_clock::duration>(target_time - now);

    auto timer = std::make_shared<boost::asio::steady_timer>(Async::Context(), wait_duration);

    timer->async_wait([&func, timer](const boost::system::error_code& ec) {
        if (ec) {
            Log("Main", Error, "OnNewDay failed %", ec.message());

            return;
        }

        std::call(func);

        OnNewDay(func);
    });
}

int main(int argc, char* argv[])
{
    if (argc >= 2) {
        std::filesystem::current_path(argv[1]);
    }

	s_Logger = std::make_unique<SimpleTgLogger>(Config::LogToken, Config::LogChat, Config::DebugBotName, Config::LogTopic);
	s_Logger->SetEnabled(Config::LogIsEnabled);

    PrinterProxy proxy;
    proxy.Listen(2228);

    proxy.RunAsync();

	Log("Main", Info, "Started");

    Async::Run();

    return 0;
}
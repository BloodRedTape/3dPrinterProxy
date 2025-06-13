#include <filesystem>
#include <bsl/log.hpp>
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
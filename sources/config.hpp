#pragma once

#include "pch/std.hpp"

struct Config {
	static std::int64_t LogChat;
	static std::int64_t LogTopic;
	static std::string LogToken;
	static std::string DebugBotName;
	static bool LogIsEnabled;

	static std::string FrontentPath;
};

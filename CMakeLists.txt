cmake_minimum_required(VERSION 3.10)
cmake_policy(SET CMP0091 NEW)

project(3dPrinterProxy)

set(BUILD_TESTING OFF)

find_package(Boost REQUIRED)
find_package(stb REQUIRED)
find_package(OpenSSL REQUIRED)
set(BSL_WITH_EXTERNAL_LOG_FUNCTION ON)
add_subdirectory(libs/bsl)
add_subdirectory(libs/json)
add_subdirectory(libs/base64)
add_subdirectory(libs/tgbot-cpp)
add_subdirectory(libs/miniz)
set(BEAUTY_BUILD_EXAMPLES OFF)
add_subdirectory(libs/beauty)
add_subdirectory(libs/SimpleTgUtils)

add_executable(3dPrinterProxy
	"./sources/main.cpp"
	"./sources/config.cpp"
	"./sources/printer_proxy.cpp"
	"./sources/printers/shui/printer.cpp"
	"./sources/printers/shui/gcode.cpp" 
	"./sources/printers/shui/storage.cpp" 
	"./sources/printers/shui/connection.cpp"
	"./sources/printers/shui/upload.cpp"
	"./sources/printers/shui/runtime_data.cpp"
	"./sources/core/async.cpp"  
	"./sources/printers/printer.cpp"  
	"./sources/interfaces/octo_print.cpp" 
	"./sources/core/image.cpp" 
	"./sources/core/base64.cpp" 
	"./sources/printers/file.cpp" 
 "sources/printers/shui/history.cpp")

target_link_libraries(3dPrinterProxy 
	PUBLIC bsl
	PUBLIC beauty::beauty 
	PUBLIC boost::boost 
	PUBLIC openssl::openssl 
	PUBLIC nlohmann_json::nlohmann_json
	PUBLIC stb::stb
	PUBLIC base64
	PUBLIC TgBot
	PUBLIC SimpleTgUtils
)

target_precompile_headers(3dPrinterProxy PRIVATE 
	"./sources/pch/asio.hpp"
	"./sources/pch/beauty.hpp"
	"./sources/pch/std.hpp"
	"./sources/pch/json.hpp"
)

target_compile_features(3dPrinterProxy PRIVATE cxx_std_20)

target_include_directories(3dPrinterProxy PRIVATE "./sources")
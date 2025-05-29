cmake_minimum_required(VERSION 3.10)
cmake_policy(SET CMP0091 NEW)

project(3dPrinterProxy)


find_package(beauty REQUIRED)
find_package(Boost REQUIRED)
find_package(stb REQUIRED)
add_subdirectory(libs/bsl)
add_subdirectory(libs/json)
add_subdirectory(libs/base64)

add_executable(3dPrinterProxy
	"./sources/main.cpp"
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
)

target_link_libraries(3dPrinterProxy 
	PUBLIC bsl
	PUBLIC beauty::beauty 
	PUBLIC boost::boost 
	PUBLIC nlohmann_json::nlohmann_json
	PUBLIC stb::stb
	PUBLIC base64
)

target_precompile_headers(3dPrinterProxy PRIVATE 
	"./sources/pch/asio.hpp"
	"./sources/pch/beauty.hpp"
	"./sources/pch/std.hpp"
	"./sources/pch/json.hpp"
)

target_compile_features(3dPrinterProxy PRIVATE cxx_std_17)

target_include_directories(3dPrinterProxy PRIVATE "./sources")
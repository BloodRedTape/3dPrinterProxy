cmake_minimum_required(VERSION 3.10)
cmake_policy(SET CMP0091 NEW)

project(3dPrinterProxy)


find_package(beauty REQUIRED)
find_package(Boost REQUIRED)
find_package(nlohmann_json REQUIRED)
add_subdirectory(libs/bsl)

add_executable(3dPrinterProxy
	"./sources/main.cpp"
	"./sources/printer_proxy.cpp"
	"./sources/printers/shui_printer.cpp"
)

target_link_libraries(3dPrinterProxy 
	PUBLIC bsl
	PUBLIC beauty::beauty 
	PUBLIC boost::boost 
	PUBLIC nlohmann_json::nlohmann_json
)

target_include_directories(3dPrinterProxy PRIVATE "./sources")
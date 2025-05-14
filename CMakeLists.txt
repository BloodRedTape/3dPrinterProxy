cmake_minimum_required(VERSION 3.10)
cmake_policy(SET CMP0091 NEW)

project(3dPrinterProxy)


find_package(beauty REQUIRED)
find_package(Boost REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(nlohmann_json REQUIRED)
add_subdirectory(libs/bsl)

add_executable(3dPrinterProxy
	"./sources/main.cpp"
)

target_link_libraries(3dPrinterProxy 
	PUBLIC bsl
	PUBLIC beauty::beauty 
	PUBLIC boost::boost 
	PUBLIC openssl::openssl 
	PUBLIC nlohmann_json::nlohmann_json
)
#include "storage.hpp"
#include <bsl/defer.hpp>
#include <unordered_set>
#include <filesystem>
#include "upload.hpp"
#include <bsl/log.hpp>
#include "pch/std.hpp"

ShuiPrinterStorage::ShuiPrinterStorage(const std::string& ip):
    m_Ip(ip)
{}

void ShuiPrinterStorage::UploadGCodeFileAsync(const std::string& filename, const std::string& content, std::function<void(bool)> callback) {

    auto OnUploaded = [=](bool success, std::string result) {
        defer{
            Println("Result %, Filename: %, 8.3: %", result, filename, *Get83Filename(filename));
            std::call(callback, success);
        };

        if (!success) 
            return;

        if(ExistsLong(filename)){
            //overwrite
            Get83Filename(filename);
        }else{
            std::string _83 = Get83(filename);

            if (!_83.size()) {
                std::call(callback, false);
                return;
            }

            m_83ToLongFilename.emplace(_83, filename);
        }
        

    };

    std::make_shared<ShuiUpload>(m_Ip, filename, content, true, OnUploaded)->RunAsync();
}

const std::string* ShuiPrinterStorage::GetLongFilename(const std::string& _83_name)const {
	auto it = m_83ToLongFilename.find(_83_name);

	if(it == m_83ToLongFilename.end())
		return nullptr;

	return &it->second;
}

const std::string* ShuiPrinterStorage::Get83Filename(const std::string& long_filename) const {
    for (const auto& [_83, _long] : m_83ToLongFilename) {
        if(_long == long_filename)
            return &_83;
    }
    return nullptr;
}

bool ShuiPrinterStorage::ExistsLong(const std::string& long_filename) const{
    return Get83Filename(long_filename);
}

bool ShuiPrinterStorage::Exists83(const std::string& _83_filename) const{
    return GetLongFilename(_83_filename);
}

std::vector<std::string> ShuiPrinterStorage::GetStoredFiles83() const{
    std::vector<std::string> result;

    for (const auto &[_83, _long] : m_83ToLongFilename) 
        result.push_back(_83);

    return result;
}

std::string ShuiPrinterStorage::Get83(const std::string& long_filename) const{
    {
        auto *_83 = Get83Filename(long_filename);

        if(_83)
            return *_83;
    }

    std::string result;
    
    for (std::int16_t revision = 0; ; revision++) {
        result = ConvertTo83Revisioned(long_filename, revision);

        if(!result.size())
            return "";

        if(!Exists83(result))
            return result;
    }

    return "";
}

std::string ShuiPrinterStorage::ConvertTo83Revisioned(const std::string& long_filename, std::int16_t revision) const{
    auto toUppercase = [](std::string& str) {
        std::transform(str.begin(), str.end(), str.begin(), ::toupper);
    };

    std::string base = std::filesystem::path(long_filename).stem().string();
    std::string extension = std::filesystem::path(long_filename).extension().string();

    if(extension.size() < 4)
        return {};

    while(base.size() < 8)
        base.push_back('_');

    while(base.size() > 8)
        base.pop_back();

    while(extension.size() > 4)
        extension.pop_back();

    toUppercase(base);
    toUppercase(extension);

    if(revision == 0)
        return base + extension;
        
    if (revision == 1) {
        base.pop_back();
        base.push_back('~');
        return base + extension;
    }

    std::string revision_prefix = Format("~%", revision - 2);
    
    //Limit now to only 4 digits or 999 + 2 revisions
    if(revision_prefix.size() > 4)
        return "";

    while(base.size() + revision_prefix.size() > 8)
        base.pop_back();

    return base + revision_prefix + extension;
}


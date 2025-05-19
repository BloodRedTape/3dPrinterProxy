#include "storage.hpp"
#include <bsl/defer.hpp>
#include <bsl/file.hpp>
#include <unordered_set>
#include <filesystem>
#include "upload.hpp"
#include <bsl/log.hpp>
#include "pch/std.hpp"

DEFINE_LOG_CATEGORY(ShuiStorage)

static GCodeFileRuntimeData ParseFromFile(const std::string& content) {
    std::stringstream stream(content);

    GCodeFileRuntimeData result;
    
    GCodeRuntimeState state;

    std::string line;
    while (std::getline(stream, line)) {
        static constexpr const char *SetPrintProgressPrefix = "M73 P";
        static constexpr const char *SetPrintLayerPrefix = "M2033.1 L";
        static constexpr const char *SetPrintHeightPrefix = ";Z:";

        GCodeRuntimeState new_state = state;

        std::string::size_type pos = line.find(SetPrintProgressPrefix);
        
        if (pos != std::string::npos){
        
            try{
                new_state.Percent = std::stoi(line.substr(pos + std::strlen(SetPrintProgressPrefix)));
            }catch(...){ }
        }

        pos = line.find(SetPrintLayerPrefix);
        
        if (pos != std::string::npos){
        
            try{
                new_state.Layer = std::stoi(line.substr(pos + std::strlen(SetPrintLayerPrefix)));
            }catch(...){ }
        }

        pos = line.find(SetPrintHeightPrefix);
        
        if (pos != std::string::npos){
        
            try{
                new_state.Height = std::round(std::stof(line.substr(pos + std::strlen(SetPrintHeightPrefix))) * 10.f) / 10.f;
            }catch(...){ }
        }

        if (new_state != state) {
            state = new_state;
            result.Index.push_back(stream.tellg());
            result.States.push_back(state);
        }
    }

    return result;
}

ShuiPrinterStorage::ShuiPrinterStorage(const std::string& ip, const std::string &filepath):
    m_Ip(ip),
    m_Filepath(filepath)
{
    if (!LoadFromFile()) {
        LogShuiStorage(Warning, "Can't load storage from file");
    }
}

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
        
        //XXX replace with bette
        std::size_t hash = std::hash<std::string>()(content);
        auto runtime_data = ParseFromFile(content);

        m_ContentHashToRuntimeData.emplace(hash, std::move(runtime_data));

        if(m_FilenameToFile.count(filename))
            m_ContentHashToRuntimeData.erase(m_FilenameToFile.at(filename).ContentHash);
            
        m_FilenameToFile.emplace(filename, GCodeFile{hash});

        SaveToFile();
    };

    std::make_shared<ShuiUpload>(m_Ip, filename, content, true, OnUploaded)->RunAsync();
}

const GCodeFileRuntimeData* ShuiPrinterStorage::GetRuntimeData(const std::string& long_filename) const{
    if(!m_FilenameToFile.count(long_filename) || !m_ContentHashToRuntimeData.count(m_FilenameToFile.at(long_filename).ContentHash))
        return nullptr;

    return &m_ContentHashToRuntimeData.at(m_FilenameToFile.at(long_filename).ContentHash);
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

void ShuiPrinterStorage::SaveToFile()const{
    WriteEntireFile(m_Filepath, nlohmann::json(*this).dump());
}

bool ShuiPrinterStorage::LoadFromFile(){
    auto content = ReadEntireFile(m_Filepath);
    
    try{
        auto json = nlohmann::json::parse(content, nullptr, false, false);
        
        from_json(json, *this);

        return true;
    }catch (...) {
        return false;
    }
}


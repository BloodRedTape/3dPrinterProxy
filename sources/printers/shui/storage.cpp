#include "storage.hpp"
#include <bsl/defer.hpp>
#include <bsl/file.hpp>
#include <unordered_set>
#include <filesystem>
#include "upload.hpp"
#include <bsl/log.hpp>
#include "pch/std.hpp"

#define WITH_PROFILE 1
#include "core/perf.hpp"
#define WITH_UPDATE_METADATA_BEFORE_UPLOAD 1

DEFINE_LOG_CATEGORY(ShuiStorage)

class StringStream {
    const std::string &m_Input;
    std::size_t m_ReadPosition = 0;
public:
    StringStream(const std::string &input):
        m_Input(input)
    {}


    std::optional<std::string_view> GetLine(char delimiter = '\n'){
        if(m_ReadPosition >= m_Input.size())
            return std::nullopt;

        std::size_t end = m_ReadPosition;

        while(end < m_Input.size() && m_Input[end] != delimiter)
            end++;

        std::string_view result(&m_Input[m_ReadPosition], &m_Input[end]);

        m_ReadPosition = end + 1;

        return result;
    }

    std::size_t ReadPosition()const{
        return m_ReadPosition;
    }
};

static GCodeFileRuntimeData ParseFromFile(const std::string& content) {
    StringStream stream(content);

    GCodeFileRuntimeData result;
    
    GCodeRuntimeState state;

    while (auto line_opt = stream.GetLine()) {
        std::string_view line = line_opt.value();

        static constexpr const char *SetPrintProgressPrefix = "M73 P";
        static constexpr const char *SetPrintLayerPrefix = "M2033.1 L";
        static constexpr const char *SetPrintHeightPrefix = ";Z:";

        GCodeRuntimeState new_state = state;

        if (line.starts_with(SetPrintProgressPrefix)){
            auto text = std::string(line.substr(std::strlen(SetPrintProgressPrefix)));
            new_state.Percent = std::stoi(text);
        }
        
        if (line.starts_with(SetPrintLayerPrefix)){
            auto text = std::string(line.substr(std::strlen(SetPrintLayerPrefix)));
            new_state.Layer = std::stoi(text);
        }
        
        if (line.starts_with(SetPrintHeightPrefix)){
            auto text = std::string(line.substr(std::strlen(SetPrintHeightPrefix)));
            new_state.Height = std::round(std::stof(text) * 10.f) / 10.f;
        }

        if (new_state != state) {
            state = new_state;
            result.Index.push_back(stream.ReadPosition());
            result.States.push_back(state);
        }
    }

    return result;
}

ShuiPrinterStorage::ShuiPrinterStorage(const std::string& ip, const std::filesystem::path &data_path):
    m_Ip(ip),
    m_DataPath(data_path)
{
    if (!LoadFromFile()) {
        LogShuiStorage(Warning, "Can't load storage from file");
    }
}

void ShuiPrinterStorage::UploadGCodeFileAsync(const std::string& filename, const std::string& content, bool print, std::function<void(bool)> callback) {

#if WITH_UPDATE_METADATA_BEFORE_UPLOAD
    OnFileUploaded(filename, content);
#endif

    auto OnUploaded = [=](bool success, std::string result) {
        defer{
            Println("Result %, Filename: %, 8.3: %", result, filename, std::safe(Get83Filename(filename)));
            std::call(callback, success);
        };
        
#if !WITH_UPDATE_METADATA_BEFORE_UPLOAD
        if(success)
            OnFileUploaded(filename, content);
#endif
    };

    std::make_shared<ShuiUpload>(m_Ip, filename, content, print, OnUploaded)->RunAsync();
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

bool ShuiPrinterStorage::OnFileUploaded(const std::string& filename, const std::string& content){
    PROFILE_SCOPE(ShuiPrinterStorage, OnFileUploaded);

    //Overwrite old file
    if(ExistsLong(filename)){
        if(m_FilenameToFile.count(filename)) {
            m_ContentHashToRuntimeData.erase(m_FilenameToFile.at(filename).ContentHash);
        }
    //Crete new file
    }else{
        std::string _83 = Get83(filename);

        if (!_83.size()) 
            return false;

        m_83ToLongFilename.emplace(_83, filename);
    }
    
    //XXX replace with bette
    std::size_t hash = 0;
    {
        PROFILE_SCOPE(ShuiPrinterStorage, OnFileUploaded_Hash);
        hash = std::hash<std::string>()(content);
    } 
    GCodeFileRuntimeData runtime_data;
    {
        PROFILE_SCOPE(ShuiPrinterStorage, OnFileUploaded_ParseRuntimeData);
        runtime_data = ParseFromFile(content);
    }

    m_ContentHashToRuntimeData.emplace(hash, std::move(runtime_data));
    
    m_FilenameToFile.emplace(filename, GCodeFile{hash});

    {
        PROFILE_SCOPE(ShuiPrinterStorage, OnFileUploaded_SaveToFile);
        SaveToFile();
    }

    return true;
}

static std::string DbFilepath(const std::filesystem::path& data_path) {
    return (data_path / "all.json").string();
}

void ShuiPrinterStorage::SaveToFile()const{
    WriteEntireFile(DbFilepath(m_DataPath), nlohmann::json(*this).dump());
}

bool ShuiPrinterStorage::LoadFromFile(){
    auto content = ReadEntireFile(DbFilepath(m_DataPath));
    
    try{
        auto json = nlohmann::json::parse(content, nullptr, false, false);
        
        from_json(json, *this);

        return true;
    }catch (...) {
        return false;
    }
}


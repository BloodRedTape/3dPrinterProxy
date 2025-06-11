#include "storage.hpp"
#include <bsl/defer.hpp>
#include <bsl/file.hpp>
#include <unordered_set>
#include <filesystem>
#include "upload.hpp"
#include <bsl/log.hpp>
#include "pch/std.hpp"
#include "core/image.hpp"
#include "core/string_utils.hpp"
    
#define WITH_PROFILE 1
#include "core/perf.hpp"

DEFINE_LOG_CATEGORY(ShuiStorage)

ShuiPrinterStorage::ShuiPrinterStorage(const std::string& ip, const std::filesystem::path &data_path):
    m_Ip(ip),
    m_DataPath(data_path)
{
    Load();
}

static std::string NoError = "";

void ShuiPrinterStorage::UploadGCodeFileAsync(const std::string& filename, const std::string& content, bool print, std::function<void(bool)> callback) {
    //risky, but Shui protocol is garbage, so as is.
    OnFileUploaded(filename, content);

    auto OnUploaded = [this, callback = std::move(callback), filename = filename](std::variant<std::string, const std::string *> result) {

        const std::string &error = result.index() == 0 ? std::get<0>(result) : NoError;
        const std::string *content = result.index() == 1 ? std::get<1>(result) : nullptr;

        Println("Error [%], Filename: %, 8.3: %", error, filename, std::safe(Get83Filename(filename)));

        std::call(callback, (bool)content);
    };

    ShuiUpload::RunAsync(m_Ip, filename, PreprocessGCode(content), print, OnUploaded);
}

bool ShuiPrinterStorage::UploadGCodeFile(const std::string& filename, const std::string& content, bool print){
    std::string processed_gcode = PreprocessGCode(content);
    //risky, but Shui protocol is garbage, so as is.
    OnFileUploaded(filename, processed_gcode);

    std::optional<std::string> result = ShuiUpload::Run(m_Ip, filename, processed_gcode, print);

    Println("Error [%], Filename: %, 8.3: %", result.value_or(""), filename, std::safe(Get83Filename(filename)));

    return !result.has_value();
}


const GCodeFileMetadata* ShuiPrinterStorage::GetMetadata(const std::string& long_filename)const {
    const std::string *_83 = Get83Filename(long_filename);

    if(!_83)
        return nullptr;
    
    if(!m_83ToFile.count(*_83))
        return nullptr;

    const GCodeFileEntry& file = m_83ToFile.at(*_83);
    
    if(!file.Metadata.count(file.ContentHash))
        return nullptr;

    return &file.Metadata.at(file.ContentHash);
}

std::string ShuiPrinterStorage::PreprocessGCode(const std::string &content) {
    static std::string ShuiPreviewStart50 = ";SHUI PREVIEW 50x50\n";
    static std::string ShuiPreviewStart100 = ";SHUI PREVIEW 100x100\n";
    static std::string ShuiPreviewEnd = ";End of SHUI PREVIEW\n";

    std::vector<Image> previews = GCodeFileMetadata::GetPreviews(content);
    
    if(!previews.size())
        return content;
    
    // biggest one
    Image &base = previews.back();

    return ShuiPreviewStart50 + base.Resize(50,50).ToSHUI() 
        + base.Resize(200, 200).ToSHUI() + ShuiPreviewEnd 
        + content;
}

const GCodeFileRuntimeData* ShuiPrinterStorage::GetRuntimeData(const std::string& long_filename) const{
    const std::string *_83 = Get83Filename(long_filename);

    if(!_83)
        return nullptr;
    
    if(!m_83ToFile.count(*_83))
        return nullptr;

    const GCodeFileEntry& file = m_83ToFile.at(*_83);

    return &file.RuntimeData;
}

const std::string* ShuiPrinterStorage::GetLongFilename(const std::string& _83_name)const {
	auto it = m_83ToFile.find(_83_name);

	if(it == m_83ToFile.end())
		return nullptr;

	return &it->second.LongFilename;
}

const std::string* ShuiPrinterStorage::Get83Filename(const std::string& long_filename) const {
    for (const auto& [_83, file] : m_83ToFile) {
        if(file.LongFilename == long_filename)
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

    for (const auto &[_83, _] : m_83ToFile) 
        result.push_back(_83);

    return result;
}

std::string ShuiPrinterStorage::Make83Filename(const std::string& long_filename) const{
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
    
    if(!ExistsLong(filename)){
        std::string _83 = Make83Filename(filename);

        if (!_83.size()){
            LogShuiStorage(Error, "Can't generate 8.3 filename for '%'", filename);
            return false;
        }
        
        GCodeFileEntry entry;
        entry.LongFilename = filename;
        m_83ToFile.emplace(_83, std::move(entry));
    }
    
    const std::string* _83_ptr = Get83Filename(filename);

    if (!_83_ptr) {
        LogShuiStorage(Error, "83 filename for '%' was not ever created", filename);
        return false;
    }

    std::string _83 = *_83_ptr;

    GCodeFileEntry& entry = m_83ToFile.at(_83);
    
    {
        PROFILE_SCOPE(ShuiPrinterStorage, OnFileUploaded_Hash);
        entry.ContentHash = std::hash<std::string>()(content);
    } 

    {
        PROFILE_SCOPE(ShuiPrinterStorage, OnFileUploaded_ParseRuntimeData);
        entry.RuntimeData = GCodeFileRuntimeData::Parse(content);
    }
    
    entry.Metadata.emplace(entry.ContentHash, GCodeFileMetadata::Parse(content));

    {
        PROFILE_SCOPE(ShuiPrinterStorage, OnFileUploaded_SaveToFile);
        Save(entry, _83);
    }

    return true;
}

std::optional<GCodeFileEntry> GCodeFileEntry::LoadFromDir(std::filesystem::path directory) {
    
    try{
        GCodeFileEntry entry = nlohmann::json::parse(ReadEntireFile((directory / "entry.json").string()), nullptr, false, false);
        
        return entry;
    }catch (...) {
        return std::nullopt;
    }
}

void GCodeFileEntry::SaveToDir(const std::filesystem::path& directory)const{
    WriteEntireFile((directory / "entry.json").string(), nlohmann::json(*this).dump());
}

void ShuiPrinterStorage::Save(const GCodeFileEntry& entry, const std::string& _83)const {
    auto entry_path = m_DataPath / _83;
    std::filesystem::create_directories(entry_path);
    
    entry.SaveToDir(entry_path);
}

void ShuiPrinterStorage::Load() {
    for (auto dir_entry : std::filesystem::directory_iterator(m_DataPath)) {
        if(!dir_entry.is_directory())
            continue;

        std::string _83 = dir_entry.path().filename().string();

        auto entry = GCodeFileEntry::LoadFromDir(dir_entry.path());

        if(!entry.has_value())
            continue;
        
        m_83ToFile.emplace(_83, std::move(entry.value()));
    }
}


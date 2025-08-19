#include "storage.hpp"
#include <bsl/defer.hpp>
#include <bsl/file.hpp>
#include <unordered_set>
#include <filesystem>
#include "upload.hpp"
#include <bsl/log.hpp>
#include <bsl/parse.hpp>
#include "pch/std.hpp"
#include "core/image.hpp"
#include "core/string_utils.hpp"
    
#define WITH_PROFILE 1
#include "core/perf.hpp"

DEFINE_LOG_CATEGORY(ShuiStorage)

ShuiPrinterStorage::ShuiPrinterStorage(const std::string& ip, const std::filesystem::path &data_path):
    m_Ip(ip),
    m_OldPath(data_path),
    m_FilesPath(data_path / "files"),
    m_MetadataPath(data_path / "metadata")
{
    std::filesystem::create_directories(m_FilesPath);
    std::filesystem::create_directories(m_MetadataPath);

    Load();
}

static std::string NoError = "";

void ShuiPrinterStorage::UploadGCodeFileAsync(const std::string& filename, const std::string& content, bool print, std::function<void(bool)> callback) {
    auto OnUploaded = [this, callback = std::move(callback), filename = filename](std::variant<std::string, const std::string *> result) {

        const std::string &error = result.index() == 0 ? std::get<0>(result) : NoError;
        const std::string *content = result.index() == 1 ? std::get<1>(result) : nullptr;

        if(content)
            OnFileUploaded(filename, *content);

        Println("Error [%], Filename: %, 8.3: %", error, filename, std::safe(Get83Filename(filename)));

        std::call(callback, (bool)content);
    };

    ShuiUpload::RunAsync(m_Ip, filename, PreprocessGCode(content), print, OnUploaded);
}

bool ShuiPrinterStorage::UploadGCodeFile(const std::string& filename, const std::string& content, bool print){
    std::string processed_gcode = PreprocessGCode(content);

    std::optional<std::string> result = ShuiUpload::Run(m_Ip, filename, processed_gcode, print);

    bool success = !result.has_value();

    if(success)
        OnFileUploaded(filename, processed_gcode);
    
    if(result.has_value())
        LogShuiStorage(Error, "UploadGCode exited with error '%'", result.value());

    Println("Error [%], Filename: %, 8.3: %", result.value_or(""), filename, std::safe(Get83Filename(filename)));

    return success;
}


const GCodeFileMetadata* ShuiPrinterStorage::GetMetadata(std::size_t content_hash) const{
    if(!m_ContentHashToMetadata.count(content_hash))
        return nullptr;

    return &m_ContentHashToMetadata.at(content_hash);
}

const GCodeFileMetadata* ShuiPrinterStorage::GetMetadata(const std::string& long_filename)const {
    auto hash = GetContentHashForFilename(long_filename);

    if(!hash)
        return nullptr;
    
    return GetMetadata(*hash);
}

std::optional<std::size_t> ShuiPrinterStorage::GetContentHashFor83Filename(const std::string& _83_filename) const{
    if(!m_83ToFile.count(_83_filename))
        return std::nullopt;
    const auto &file = m_83ToFile.at(_83_filename);

    return file.ContentHash;
}

std::optional<std::size_t> ShuiPrinterStorage::GetContentHashForFilename(const std::string& filename) const{
    const std::string *_83 = Get83Filename(filename);

    if(!_83)
        return std::nullopt;
    
    return GetContentHashFor83Filename(*_83);
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

    for (char& ch : base) {
        if(ch == ' ')
            ch= '_';
    }

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
    
    {
        PROFILE_SCOPE(ShuiPrinterStorage, OnFileUploaded_ParseFileMetadata);
        m_ContentHashToMetadata[entry.ContentHash] = GCodeFileMetadata::ParseFromGCode(content);
    }

    {
        PROFILE_SCOPE(ShuiPrinterStorage, OnFileUploaded_SaveToFile);
        Save(entry, _83);
    }

    return true;
}

std::optional<GCodeFileEntry> GCodeFileEntry::LoadFromFile(std::filesystem::path filepath) {
    try{
        GCodeFileEntry entry = nlohmann::json::parse(File::ReadEntire(filepath), nullptr, false, false);
        
        return entry;
    }catch (...) {
        return std::nullopt;
    }
}

void GCodeFileEntry::SaveToFile(const std::filesystem::path& filepath)const{
    File::WriteEntire(filepath, nlohmann::json(*this).dump());
}

void ShuiPrinterStorage::Save(const GCodeFileEntry& entry, const std::string& _83)const {
    auto entry_path = m_FilesPath / _83;

    std::filesystem::create_directories(m_FilesPath);
    
    entry.SaveToFile(entry_path);
}

void ShuiPrinterStorage::Load() {
    for (auto file_entry: std::filesystem::directory_iterator(m_FilesPath)) {
        if(!file_entry.is_regular_file())
            continue;

        std::string _83 = file_entry.path().filename().string();

        auto entry = GCodeFileEntry::LoadFromFile(file_entry.path());

        if(!entry.has_value())
            continue;
        
        m_83ToFile.emplace(_83, std::move(entry.value()));
    }

    for (auto file_entry: std::filesystem::directory_iterator(m_MetadataPath)) {
        if(!file_entry.is_regular_file())
            continue;

        auto hash = FromString<std::size_t>(file_entry.path().filename().string());

        if(!hash.has_value())
            continue;

        auto data = GCodeFileMetadata::ParseFromJsonFile(file_entry.path());

        if(!data.has_value())
            continue;
        
        m_ContentHashToMetadata.emplace(hash.value(), std::move(data.value()));
    }
}


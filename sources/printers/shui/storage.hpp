#pragma once

#include "printers/file.hpp"
#include "printers/storage.hpp"
#include "runtime_data.hpp"

struct GCodeFileEntry {
	std::string LongFilename;
	std::size_t ContentHash;
	GCodeFileRuntimeData RuntimeData;

	std::unordered_map<std::size_t, GCodeFileMetadata> Metadata;

	NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(GCodeFileEntry, LongFilename, ContentHash, RuntimeData, Metadata);

	static std::optional<GCodeFileEntry> LoadFromFile(std::filesystem::path filepath);

	void SaveToFile(const std::filesystem::path &filepath)const;
};

class ShuiPrinterStorage: public PrinterStorage{
	std::string m_Ip;
	std::filesystem::path m_OldPath;
	std::filesystem::path m_FilesPath;
	std::filesystem::path m_MetadataPath;
	
	std::unordered_map<std::string, GCodeFileEntry> m_83ToFile;

	std::unordered_map<std::size_t, GCodeFileMetadata> m_ContentHashToMetadata;
public:
	ShuiPrinterStorage(const std::string& ip, const std::filesystem::path &data_path);

	void UploadGCodeFileAsync(const std::string &filename, const std::string& content, bool print, std::function<void(bool)> callback)override;

	bool UploadGCodeFile(const std::string &filename, const std::string& content, bool print)override;

	//GCodeFile *GetStoredFile(const std::string &filename)const;

	const GCodeFileMetadata *GetMetadata(std::size_t content_hash)const override;

	const GCodeFileMetadata *GetMetadata(const std::string& filename)const override;

	std::optional<std::size_t> GetContentHashForFilename(const std::string &filename)const override;

	std::optional<std::size_t> GetContentHashFor83Filename(const std::string &_83_filename)const;

	//std::vector<std::string> GetStoredFiles()const;

	std::string PreprocessGCode(const std::string &content);

	const GCodeFileRuntimeData *GetRuntimeData(const std::string &long_filename)const;

	const std::string *GetLongFilename(const std::string &_83_filename)const;

	const std::string *Get83Filename(const std::string &long_filename)const;

	bool ExistsLong(const std::string &long_filename)const;

	bool Exists83(const std::string &_83_filename)const;

	std::vector<std::string> GetStoredFiles83()const;

	std::string Make83Filename(const std::string& long_filename)const;

	std::string ConvertTo83Revisioned(const std::string& long_filename, std::int16_t revision)const;

private:
	bool OnFileUploaded(const std::string &filename, const std::string &content);

	void Save(const GCodeFileEntry& entry, const std::string& _83)const;

	void Load();
};

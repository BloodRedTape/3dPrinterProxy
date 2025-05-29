#pragma once

#include "printers/file.hpp"
#include "printers/storage.hpp"
#include "runtime_data.hpp"

struct GCodeFileEntry {
	std::string LongFilename;
	GCodeFileRuntimeData RuntimeData;
	std::size_t ContentHash;

	std::unordered_map<std::size_t, GCodeFileMetadata> Metadata;

	NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(GCodeFileEntry, LongFilename, RuntimeData, ContentHash, Metadata);

	static std::optional<GCodeFileEntry> LoadFromDir(std::filesystem::path directory);

	void SaveToDir(const std::filesystem::path &directory)const;
};

class ShuiPrinterStorage: public PrinterStorage{
	std::string m_Ip;
	std::filesystem::path m_DataPath;
	
	std::unordered_map<std::string, GCodeFileEntry> m_83ToFile;
public:
	ShuiPrinterStorage(const std::string& ip, const std::filesystem::path &data_path);

	void UploadGCodeFileAsync(const std::string &filename, const std::string& content, bool print, std::function<void(bool)> callback)override;

	//GCodeFile *GetStoredFile(const std::string &filename)const;

	const GCodeFileMetadata *GetMetadata(const std::string& filename)const override;

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

	bool Load();
};

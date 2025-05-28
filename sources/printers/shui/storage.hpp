#pragma once

#include "printers/file.hpp"
#include "printers/storage.hpp"

struct GCodeFile {
	std::size_t ContentHash = 0;

	NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(GCodeFile, ContentHash)
};

class ShuiPrinterStorage: public PrinterStorage{
	std::string m_Ip;
	std::filesystem::path m_DataPath;

	//std::unordered_map<std::size_t, GCodeFileMetadata> m_ContentHashToMetadata;
	std::unordered_map<std::size_t, GCodeFileRuntimeData> m_ContentHashToRuntimeData;
	std::unordered_map<std::string, GCodeFile> m_FilenameToFile;
	std::unordered_map<std::string, std::string> m_83ToLongFilename;
public:
	ShuiPrinterStorage(const std::string& ip, const std::filesystem::path &data_path);

	void UploadGCodeFileAsync(const std::string &filename, const std::string& content, bool print, std::function<void(bool)> callback)override;

	//GCodeFile *GetStoredFile(const std::string &filename)const;

	//const GCodeFileMetadata *GetMetadata(const std::string& content_hash)const;

	//std::vector<std::string> GetStoredFiles()const;

	std::string PreprocessGCode(const std::string &content);

	const GCodeFileRuntimeData *GetRuntimeData(const std::string &long_filename)const;

	const std::string *GetLongFilename(const std::string &_83_filename)const;

	const std::string *Get83Filename(const std::string &long_filename)const;

	bool ExistsLong(const std::string &long_filename)const;

	bool Exists83(const std::string &_83_filename)const;

	std::vector<std::string> GetStoredFiles83()const;

	std::string Get83(const std::string& long_filename)const;

	std::string ConvertTo83Revisioned(const std::string& long_filename, std::int16_t revision)const;

private:
	bool OnFileUploaded(const std::string &filename, const std::string &content);

	void SaveToFile()const;

	bool LoadFromFile();

	friend void to_json(nlohmann::json& json, const ShuiPrinterStorage& storage) {
		json["ContentHashToRuntimeData"] = storage.m_ContentHashToRuntimeData;
		json["FilenameToFile"] = storage.m_FilenameToFile;
		json["_83ToLongFilename"] = storage.m_83ToLongFilename;
	}

	friend void from_json(const nlohmann::json& json, ShuiPrinterStorage& storage) {
		storage.m_ContentHashToRuntimeData = json["ContentHashToRuntimeData"];
		storage.m_FilenameToFile = json["FilenameToFile"];
		storage.m_83ToLongFilename = json["_83ToLongFilename"];
	}
};

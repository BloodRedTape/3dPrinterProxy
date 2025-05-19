#pragma once

#include "printers/file.hpp"

struct GCodeFile {
	std::string Filename;

	GCodeFileRuntimeData RuntimeData;

	std::string Content;
	std::string ContentHash;
};

class ShuiPrinterStorage {
	std::string m_Ip;
	//std::unordered_map<std::string, GCodeFileMetadata> m_ContentHashToMetadata;
	//std::unordered_map<std::string, GCodeFile> m_StoredFiles;

	std::unordered_map<std::string, std::string> m_83ToLongFilename;
public:
	ShuiPrinterStorage(const std::string& ip);

	void UploadGCodeFileAsync(const std::string &filename, const std::string& content, std::function<void(bool)> callback);

	//GCodeFile *GetStoredFile(const std::string &filename)const;

	//const GCodeFileMetadata *GetMetadata(const std::string& content_hash)const;

	//std::vector<std::string> GetStoredFiles()const;

	const std::string *GetLongFilename(const std::string &_83_filename)const;

	const std::string *Get83Filename(const std::string &long_filename)const;

	bool ExistsLong(const std::string &long_filename)const;

	bool Exists83(const std::string &_83_filename)const;

	std::vector<std::string> GetStoredFiles83()const;

	std::string Get83(const std::string& long_filename)const;

	std::string ConvertTo83Revisioned(const std::string& long_filename, std::int16_t revision)const;

};
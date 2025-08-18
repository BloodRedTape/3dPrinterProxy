#pragma once

#include "pch/std.hpp"
#include "pch/json.hpp"
#include "core/image.hpp"

struct GCodeFileFilament {
	std::string Type;
	float PressureAdvance;
	//Color
	float Cost;

	float EstimatedUseMM;
	float EstimatedUseG;
	float EstimatedCost;
};

struct GCodeFileMetadata {
	std::int64_t BytesSize = 0;

	std::vector<Image> Previews;
	//std::uint64_t SlicedAtUnixtime;
	std::int64_t EstimatedPrintTime;

	std::int32_t Layers = 0;
	float Height = 0;

	std::int32_t Toolchanges = 0;
	std::int32_t Objects = 0;
	//PrintSequence

	bool EnableSupports = false;

	float NozzleDiameter = 0;

	//float InitialExtruderTemperature;
	//float ExtruderTemperature;

	//float InitialBedTemperature;
	//float BedTemperature;

	//std::vector<GCodeFileFilament> Filaments;
	
	NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(GCodeFileMetadata, BytesSize, Previews, Layers, Height, Toolchanges, Objects, EnableSupports, NozzleDiameter, EstimatedPrintTime)

	static std::vector<Image> GetPreviews(const std::string& content);

	static GCodeFileMetadata ParseFromGCode(const std::string &content);

	static std::optional<GCodeFileMetadata> ParseFromJsonFile(const std::filesystem::path& filepath);
};


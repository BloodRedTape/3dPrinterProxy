#pragma once

#include "pch/std.hpp"


struct GCodeFileThumbnail {
	std::int32_t Width;
	std::int32_t Height;
	//std::string ImageData;
};

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
	std::string Filepath;
	std::int64_t BytesSize;

	std::vector<GCodeFileThumbnail> Thumbnails;
	std::uint64_t SlicedAtUnixtime;
	std::int64_t EstimatedPrintTime;

	std::int32_t Layers;
	float Height;

	std::int32_t Toolchanges;
	std::int32_t Objects;
	//PrintSequence

	bool EnableSupports;

	float NozzleDiameter;

	float InitialExtruderTemperature;
	float ExtruderTemperature;

	float InitialBedTemperature;
	float BedTemperature;

	std::vector<GCodeFileFilament> Filaments;
	
};

struct GCodeRuntimeState {
	float Percent;
	std::int64_t Layer;
	float Height;
	//std::int32_t FillamentIndex;
};

struct GCodeFileRuntimeData {
	//...	

	GCodeRuntimeState GetStateNear(std::int64_t printed_byte)const {
		assert(false);
		return {};
	}
};


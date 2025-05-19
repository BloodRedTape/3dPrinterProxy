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
	float Percent = 0.f;
	std::int64_t Layer = 0;
	float Height = 0.f;
	//std::int32_t FillamentIndex;

	bool operator==(const GCodeRuntimeState& other)const{
		return Percent == other.Percent && Layer == other.Layer && Height == other.Height;
	}

	bool operator!=(const GCodeRuntimeState& other)const{
		return !(*this == other);
	}
};

struct GCodeFileRuntimeData {
	std::vector<GCodeRuntimeState> States;
	//Can't imagine file more that 4gigs
	std::vector<std::int32_t> Index;

	GCodeRuntimeState GetStateNear(std::int64_t printed_byte)const {
		if(printed_byte == 0)
			return States.size() ? States.front() : GCodeRuntimeState();
		
		for (int i = 0; i<Index.size(); i++) {
			std::int32_t byte = Index[i];

			if (byte < printed_byte)
				continue;
			
			if(i >= States.size())
				return States.size() ? States.back() : GCodeRuntimeState();
			
			return States[i];
		}

		return States.size() ? States.back() : GCodeRuntimeState();
	}
};


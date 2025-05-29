#pragma once

#include "pch/std.hpp"
#include "pch/json.hpp"

struct GCodeRuntimeState {
	float Percent = 0.f;
	std::int64_t Layer = 0;
	float Height = 0.f;
	//std::int32_t FillamentIndex;

	NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(GCodeRuntimeState, Percent, Layer, Height)
	
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

	NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(GCodeFileRuntimeData, States, Index)

	GCodeRuntimeState GetStateNear(std::int64_t printed_byte)const;

	static GCodeFileRuntimeData Parse(const std::string& content);
};

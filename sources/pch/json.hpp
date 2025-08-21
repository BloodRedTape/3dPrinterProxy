#pragma once

#include <nlohmann/json.hpp>
#include <boost/algorithm/string.hpp>
#include <bsl/enum.hpp>


#define NLOHMANN_DEFINE_BSL_ENUM_WITH_DEFAULT(EnumName, DefaultValue) \
inline void to_json(nlohmann::json& json, const EnumName& value) { \
	json = value.Name(); \
} \
inline void from_json(const nlohmann::json& json, EnumName& value) { \
	value = EnumName::FromString(json.get<std::string>()).value_or(DefaultValue); \
} 
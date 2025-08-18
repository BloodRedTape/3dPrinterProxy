#pragma once

#include <string>
#include <string_view>
#include <functional>
#include <vector>
#include <unordered_map>
#include <memory>
#include <optional>
#include <cassert>
#include <vector>
#include <filesystem>
#include <variant>
#include <sstream>

namespace std {
	template<typename FunctionType, typename...ArgsType>
	inline void call(const FunctionType& func, ArgsType&&...args) {
		if(func)
			func(std::forward<ArgsType>(args)...);
	}
	
	template<typename Type>
	inline const Type& safe(const Type* ptr) {
		if(ptr)
			return *ptr;

		std::cerr << "std::safe: null value is supplied of type '" << typeid(*ptr).name() << "'\n";
		static Type empty;
		return empty;
	}

	template<typename CallableType, typename ...ArgsType>
	auto errno_call(CallableType function, ArgsType&&...args) -> std::optional<decltype(std::declval<CallableType>()(std::declval<ArgsType>()...))> {
		errno = 0;

		auto result = (*function)(std::forward<ArgsType>(args)...);

		if(errno)
			return std::nullopt;

		return std::make_optional(result);
	}
}
#pragma once

#include <string>
#include <functional>
#include <vector>
#include <unordered_map>
#include <memory>
#include <optional>
#include <cassert>
#include <vector>
#include <filesystem>

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
}
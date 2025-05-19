#pragma once

#include <string>
#include <functional>
#include <vector>
#include <unordered_map>
#include <memory>
#include <optional>
#include <cassert>
#include <vector>

namespace std {
	template<typename FunctionType, typename...ArgsType>
	inline void call(const FunctionType& func, ArgsType&&...args) {
		if(func)
			func(std::forward<ArgsType>(args)...);
	}
}
#include "file.hpp"


GCodeRuntimeState GCodeFileRuntimeData::GetStateNear(std::int64_t printed_byte)const {
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
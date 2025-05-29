#include "runtime_data.hpp"
#include "core/string_utils.hpp"

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


GCodeFileRuntimeData GCodeFileRuntimeData::Parse(const std::string& content) {
    StringStream stream(content);

    GCodeFileRuntimeData result;
    
    GCodeRuntimeState state;

    while (auto line_opt = stream.GetLine()) {
        std::string_view line = line_opt.value();

        static constexpr const char *SetPrintProgressPrefix = "M73 P";
        static constexpr const char *SetPrintLayerPrefix = "M2033.1 L";
        static constexpr const char *SetPrintHeightPrefix = ";Z:";

        GCodeRuntimeState new_state = state;

        if (line.starts_with(SetPrintProgressPrefix)){
            auto text = std::string(line.substr(std::strlen(SetPrintProgressPrefix)));
            new_state.Percent = std::stoi(text);
        }
        
        if (line.starts_with(SetPrintLayerPrefix)){
            auto text = std::string(line.substr(std::strlen(SetPrintLayerPrefix)));
            new_state.Layer = std::stoi(text);
        }
        
        if (line.starts_with(SetPrintHeightPrefix)){
            auto text = std::string(line.substr(std::strlen(SetPrintHeightPrefix)));
            new_state.Height = std::round(std::stof(text) * 10.f) / 10.f;
        }

        if (new_state != state) {
            state = new_state;
            result.Index.push_back(stream.ReadPosition());
            result.States.push_back(state);
        }
    }

    return result;
}
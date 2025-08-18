#include "file.hpp"
#include "core/string_utils.hpp"
#include <bsl/log.hpp>
#include <bsl/file.hpp>

DEFINE_LOG_CATEGORY(File)

std::vector<Image> GCodeFileMetadata::GetPreviews(const std::string& content) {
    static std::string_view ThumbnailBegin = "; thumbnail begin ";
    static std::string_view ThumbnailEnd = "; thumbnail end";
    static std::string_view LinePrefix = "; ";
    static std::string_view LineSuffix = "\n";

    std::string result;

    StringStream stream(content);

    std::vector<Image> previews;
    std::string preview_data;

    bool reading_preview = false;

    while (auto line_opt = stream.GetLine()) {
        std::string_view line = line_opt.value();

        if (line.starts_with(ThumbnailEnd)) {
            auto image = Image::LoadFromBase64(preview_data);

            if (image.has_value()) {
                previews.push_back(std::move(image.value()));
            }
            reading_preview = false;
            continue;
        }

        if (line.starts_with(ThumbnailBegin)) {
            preview_data.clear();
            reading_preview = true;
            continue;
        }

        if (reading_preview) {
            if(line.starts_with(LinePrefix))
                line = line.substr(LinePrefix.size());

            if(line.ends_with(LineSuffix))
                line = line.substr(0, line.size() - LineSuffix.size());

            preview_data.append(line);
        }
    }

    return previews;
}

GCodeFileMetadata GCodeFileMetadata::ParseFromGCode(const std::string& content) {
    GCodeFileMetadata metadata;
    metadata.BytesSize = content.size();
    metadata.Previews = GetPreviews(content);

    StringStream stream(content);

    while (auto line_opt = stream.GetLine()) {
        std::string_view line = line_opt.value();

        try{
            if (auto value = SubstrAfter(line, "; total layer number: "); value.size()) {
                metadata.Layers = std::stoi(std::string(value));
            }

            if (auto value = SubstrAfter(line, "; max_z_height: "); value.size()) {
                metadata.Height = std::stof(std::string(value));
            }

            if (auto value = SubstrAfter(line, ";TOTAL_TOOLCHANGES:"); value.size()) {
                metadata.Toolchanges = std::stoi(std::string(value));
            }

            if (auto value = SubstrAfter(line, ";NUM_INSTANCES:"); value.size()) {
                metadata.Objects = std::stoi(std::string(value));
            }

            if (auto value = SubstrAfter(line, "; enable_support = "); value.size()) {
                metadata.EnableSupports = std::stoi(std::string(value));
            }

            if (auto value = SubstrAfter(line, "; nozzle_diameter = "); value.size()) {
                metadata.NozzleDiameter = std::stof(std::string(value));
            }

            if (auto value = SubstrAfter(line, "; estimated printing time (normal mode) = "); value.size()) {
                
                std::int32_t print_time = 0;

                std::string number;
                
                for (char ch : value) {
                    if (std::isdigit(ch)) {
                        number += ch;
                    }

                    if ((ch == 'h' || ch == 'm' || ch == 's') && number.size()) {
                        
                        auto units = std::stoi(number);

                        if(ch == 's')
                            print_time += units;
                        if(ch == 'm')
                            print_time += units * 60;
                        if(ch == 'h')
                            print_time += units * 60 * 60;

                        number.clear();
                    }
                }

                metadata.EstimatedPrintTime = print_time;

            }
        }
        catch (const std::exception &e) {
            LogFile(Error, "MetadataParse failed: %", e.what());
        }
    }

    return metadata;
}

std::optional<GCodeFileMetadata> GCodeFileMetadata::ParseFromJsonFile(const std::filesystem::path& filepath){
    try{
        GCodeFileMetadata data = nlohmann::json::parse(File::ReadEntire(filepath), nullptr, false, false);
        
        return data;
    }catch (...) {
        return std::nullopt;
    }
}

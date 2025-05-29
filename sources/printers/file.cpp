#include "file.hpp"
#include "core/string_stream.hpp"

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

GCodeFileMetadata GCodeFileMetadata::Parse(const std::string& content) {
    GCodeFileMetadata metadata;
    metadata.BytesSize = content.size();
    metadata.Previews = GetPreviews(content);
    return metadata;
}

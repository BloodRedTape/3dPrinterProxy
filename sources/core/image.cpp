#include "image.hpp"
#include "base64.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize.h"
#include <boost/endian/conversion.hpp>

Image::Image(std::int32_t width, std::int32_t height, std::vector<std::uint32_t>&& data):
	m_Width(width),
	m_Height(height),
	m_Data(std::move(data))
{}

Image::Image(Image && other)noexcept{
    *this = std::move(other);
}

Image& Image::operator=(Image&& other)noexcept{
    m_Data = std::move(other.m_Data);

    m_Width = other.m_Width;
    other.m_Width = 0;

    m_Height = other.m_Height;
    other.m_Height = 0;

    return *this;
}

Image Image::Resize(std::int32_t width, std::int32_t height) const{
    std::vector<std::uint32_t> data(width * height);

    stbir_resize_uint8(
        reinterpret_cast<const unsigned char*>(m_Data.data()), // input pixels
        m_Width, m_Height, 0,                                  // input width, height, stride
        reinterpret_cast<unsigned char*>(data.data()),  // output pixels
        width, height, 0,                                // output width, height, stride
        4                                                      // number of channels (RGBA)
    );

    return Image(width, height, std::move(data));
}

std::uint32_t Image::Get(std::int32_t x, std::int32_t y) const{
    return m_Data[x + y * m_Width];
}

static std::uint16_t ToRGB565(std::uint32_t rgba){
    std::uint8_t b = (rgba >> 16) & 0xFF;  // Extract red
    std::uint8_t g = (rgba >> 8) & 0xFF;   // Extract green
    std::uint8_t r = rgba & 0xFF;          // Extract blue
    
    std::uint16_t r5 = (r >> 3) & 0x1F;    // 5 bits for red
    std::uint16_t g6 = (g >> 2) & 0x3F;    // 6 bits for green
    std::uint16_t b5 = (b >> 3) & 0x1F;    // 5 bits for blue
    
    return (r5 << 11) | (g6 << 5) | b5;
}
std::string Image::ToSHUI() const{
    static std::string ShuiPreviewLinePrefix = ";";
    static std::string ShuiPreviewLineSuffix = "\n";

    std::string result;
    for (int y = 0; y < Height(); y++) {
        std::string row;

        for (int x = 0; x < Width(); x++) {

            std::uint16_t rgb565 = ToRGB565(Get(x, y));

            rgb565 = boost::endian::native_to_big(rgb565);

            row.append((const char*)&rgb565, 2);
        }

        result += ShuiPreviewLinePrefix + Base64::Encode(row) + ShuiPreviewLineSuffix;
    }
    return result;
}

std::optional<Image> Image::LoadFromMemory(const void* memory, std::size_t size){
    int width = 0, height = 0, channels = 0;

    unsigned char* pixels = stbi_load_from_memory(
        static_cast<const stbi_uc*>(memory),
        static_cast<int>(size),
        &width,
        &height,
        &channels,
        4 
    );

    if (!pixels)
        return std::nullopt;

    std::vector<std::uint32_t> data(width * height);
    std::memcpy(data.data(), pixels, width * height * 4);

    stbi_image_free(pixels);

    return Image(width, height, std::move(data));
}

std::optional<Image> Image::LoadFromBase64(const std::string& base64){
    auto memory = Base64::Decode(base64);

    if(!memory.size())
        return std::nullopt;

    return LoadFromMemory(memory.data(), memory.size());
}

#pragma once

#include "pch/std.hpp"
#include "pch/json.hpp"

class Image {
	std::int32_t m_Width = 0;
	std::int32_t m_Height = 0;
	std::vector<std::uint32_t> m_Data;
public:
	Image() = default;

	Image(const Image &) = default;

	Image(std::int32_t width, std::int32_t height, std::vector<std::uint32_t> &&data);

	Image(Image &&other)noexcept;

	Image &operator=(const Image &) = default;

	Image &operator=(Image &&other)noexcept;

	std::int32_t Width()const{return m_Width;}

	std::int32_t Height()const{return m_Height;}

	Image Resize(std::int32_t width, std::int32_t height)const;

	std::uint32_t Get(std::int32_t x, std::int32_t y)const;

	std::string ToSHUI()const;

	std::string ToBase64()const;

	std::string ToPng()const;

	static std::optional<Image> LoadFromMemory(const void *memory, std::size_t size);

	static std::optional<Image> LoadFromBase64(const std::string& base64);

	friend void to_json(nlohmann::json &json, const Image &image);
	friend void from_json(const nlohmann::json &json, Image &image);
};
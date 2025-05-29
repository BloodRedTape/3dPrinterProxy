#pragma once

#include "pch/std.hpp"

class StringStream {
    const std::string &m_Input;
    std::size_t m_ReadPosition = 0;
public:
    StringStream(const std::string &input):
        m_Input(input)
    {}


    std::optional<std::string_view> GetLine(char delimiter = '\n'){
        if(m_ReadPosition >= m_Input.size())
            return std::nullopt;

        std::size_t end = m_ReadPosition;

        while(end < m_Input.size() && m_Input[end] != delimiter)
            end++;

        std::string_view result(&m_Input[m_ReadPosition], &m_Input[end]);

        m_ReadPosition = end + 1;

        return result;
    }

    std::size_t ReadPosition()const{
        return m_ReadPosition;
    }
};


inline std::string_view SubstrBy(std::string_view string, std::string_view prefix, std::string_view suffix) {
    auto rn1 = string.find(prefix);

    if(rn1 == std::string_view::npos)
        return {};

    auto rn2 = string.find(suffix, rn1 + prefix.size());

    if(rn2 == std::string_view::npos)
        return {};

    return string.substr(rn1 + prefix.size(), rn2 - rn1 - prefix.size());
}

inline std::string_view SubstrAfter(std::string_view string, std::string_view prefix) {
    auto rn1 = string.find(prefix);

    if(rn1 == std::string_view::npos)
        return {};

    return string.substr(rn1 + prefix.size());
}
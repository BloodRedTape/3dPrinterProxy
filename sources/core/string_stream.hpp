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

#include "base64.hpp"
#include "libbase64.h"

std::string Base64::Encode(const std::string& in)
{
    if (in.empty()) {
        return "";
    }
    
    size_t max_output_size = ((in.size() + 2) / 3) * 4;
    
    std::string out(max_output_size, '\0');
    size_t outlen = 0;
    
    base64_encode(in.c_str(), in.size(), &out[0], &outlen, 0);
    
    out.resize(outlen);
    
    return out;
}

std::string Base64::Decode(const std::string& in)
{
    if (in.empty()) {
        return "";
    }
    
    size_t max_output_size = (in.size() * 3) / 4 + 3; // Add some margin for safety
    
    std::string out(max_output_size, '\0');
    size_t outlen = 0;
    
    int result = base64_decode(in.c_str(), in.size(), &out[0], &outlen, 0);
    
    if (result != 1) {
        return {};
    }
    
    out.resize(outlen);
    
    return out;
}

#pragma once

#include <string>
#include "../Platform/PlatformTypes.h"
#include <type_traits>
#include <vector>

#define STRINGIFY(...) #__VA_ARGS__

class String :public std::string
{

public:
    String() :std::string() {}

    String(const String& otherString);
    String(const String& otherString,size_type pos,size_type len);
    String(const AChar* s,size_type n);
    String(const AChar* s);
    String(size_type n, AChar c);
    String(const std::string& otherString);
    String(std::string&& otherString);

    const AChar* getChar() const;
    bool findAny(size_t& outIndex, String& outFoundString, const std::vector<String>& findStrgs, size_t offset = 0, 
        bool fromEnd = false) const;

    bool operator==(const String& rhs) const;

};


template <>
struct std::hash<String> {

    _NODISCARD size_t operator()(const String keyval) const noexcept {
        auto stringHash = hash<std::string>();
        return stringHash(keyval);
    }
};
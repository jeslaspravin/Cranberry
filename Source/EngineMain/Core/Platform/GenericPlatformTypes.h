#pragma once

class GenericPlatformTypes {

public:

    typedef unsigned char uint8;
    typedef unsigned int uint32;
    typedef unsigned long long uint64;
    typedef unsigned short int uint16;


    typedef signed char int8;
    typedef signed int int32;
    typedef signed long long int64;
    typedef signed short int int16;

    typedef wchar_t WChar;
    typedef char AChar;
    typedef WChar TChar;
    typedef uint8 Char8;
    typedef uint16 Char16;
    typedef uint32 Char32;

    typedef unsigned short word;
    typedef unsigned long dword;


    typedef union _uint64 {
        struct {
            dword lowPart;
            dword highPart;
        };
        struct {
            dword lowPart;
            dword highPart;
        } u;
        uint64 quadPart;
    } UInt64;
};

struct LibPointer {

    virtual ~LibPointer() {}

};

typedef LibPointer* LibPointerPtr;

struct PlatformInstance{};


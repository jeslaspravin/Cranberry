#include "String/String.h"
#include "Types/Traits/ValueTraits.h"
#include "Types/Platform/PlatformFunctions.h"
#include "Logger/Logger.h"

#include <locale>

const AChar* StringConv<WChar, AChar>::convert(const WChar* start)
{
    if (!PlatformFunctions::wcharToUtf8(str, start))
    {
        auto& encodedToUtf8 = std::use_facet<std::codecvt<EncodedType, Utf8, std::mbstate_t>>(std::locale());
        const auto* end = String::recurseToNullEnd(start);

        // Convert from UTF-16/UTF-32 to UTF-8
        std::mbstate_t state{};
        const EncodedType* nextFrom = nullptr;
        Utf8* nextTo = nullptr;
        Utf8* outData = reinterpret_cast<Utf8*>(str.data());

        str.resize(encodedToUtf8.max_length() * (end - start), TCHAR('\0'));
        std::codecvt_base::result status = encodedToUtf8.out(state
            , reinterpret_cast<const EncodedType*>(start), reinterpret_cast<const EncodedType*>(end), nextFrom
            , outData, outData + str.size(), nextTo);
        str.resize(nextTo - outData);
        if (status != std::codecvt_base::ok)
        {
            LOG_ERROR("StringConv", "Failed to convert from WChar(UTF-16/UTF-32) to AChar(UTF-8)");
            str.clear();
        }
    }
    return str.c_str();
}

const WChar* StringConv<AChar, WChar>::convert(const AChar* start)
{
    if (!PlatformFunctions::utf8ToWChar(str, start))
    {
        auto& encodedToUtf8 = std::use_facet<std::codecvt<EncodedType, Utf8, std::mbstate_t>>(std::locale());
        const auto* end = String::recurseToNullEnd(start);

        // Convert from UTF-8 to UTF-16/UTF-32
        std::mbstate_t state{};
        const Utf8* nextFrom = nullptr;
        EncodedType* nextTo = nullptr;
        EncodedType* outData = reinterpret_cast<EncodedType*>(str.data());

        str.resize(end - start, TCHAR('\0'));
        std::codecvt_base::result status = encodedToUtf8.in(state
            , reinterpret_cast<const Utf8*>(start), reinterpret_cast<const Utf8*>(end), nextFrom
            , outData, outData + str.size(), nextTo);
        str.resize(nextTo - outData);
        if (status != std::codecvt_base::ok)
        {
            LOG_ERROR("StringConv", "Failed to convert from AChar(UTF-8) to WChar(UTF-16/UTF-32)");
            str.clear();
        }
    }
    return str.c_str();
}

const AChar* StringConv<NotEncodedCharType, AChar>::convert(const NotEncodedCharType* start)
{
    auto& nonencodedToUtf8 = std::use_facet<std::codecvt<NotEncodedCharType, Utf8, std::mbstate_t>>(std::locale());
    const auto* end = String::recurseToNullEnd(start);

    // Convert from UTF-16/UTF-32 to UTF-8
    std::mbstate_t state{};
    const NotEncodedCharType* nextFrom = nullptr;
    Utf8* nextTo = nullptr;
    Utf8* outData = reinterpret_cast<Utf8*>(str.data());

    str.resize(nonencodedToUtf8.max_length() * (end - start), TCHAR('\0'));
    std::codecvt_base::result status = nonencodedToUtf8.out(state
        , start, end, nextFrom
        , outData, outData + str.size(), nextTo);
    str.resize(nextTo - outData);
    if (status != std::codecvt_base::ok)
    {
        LOG_ERROR("StringConv", "Failed to convert from Non native platfrom encoding(UTF-16/UTF-32) to AChar(UTF-8)");
        str.clear();
    }
    return str.c_str();
}

const NotEncodedCharType* StringConv<AChar, NotEncodedCharType>::convert(const AChar* start)
{
    auto& nonencodedToUtf8 = std::use_facet<std::codecvt<NotEncodedCharType, Utf8, std::mbstate_t>>(std::locale());
    const auto* end = String::recurseToNullEnd(start);

    // Convert from UTF-8 to UTF-16/UTF-32
    std::mbstate_t state{};
    const Utf8* nextFrom = nullptr;
    NotEncodedCharType* nextTo = nullptr;
    NotEncodedCharType* outData = str.data();

    str.resize(end - start, TCHAR('\0'));
    std::codecvt_base::result status = nonencodedToUtf8.in(state
        , reinterpret_cast<const Utf8*>(start), reinterpret_cast<const Utf8*>(end), nextFrom
        , outData, outData + str.size(), nextTo);
    str.resize(nextTo - outData);
    if (status != std::codecvt_base::ok)
    {
        LOG_ERROR("StringConv", "Failed to convert from AChar(UTF-8) to WChar(UTF-16/UTF-32)");
        str.clear();
    }
    return str.c_str();
}

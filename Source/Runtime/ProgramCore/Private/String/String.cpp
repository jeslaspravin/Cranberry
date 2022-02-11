#include "String/String.h"
#include "Types/Traits/ValueTraits.h"
#include "Types/Platform/PlatformFunctions.h"
#include "Logger/Logger.h"

#include <locale>

template <typename BufferType, typename NonUtf8Type>
FORCE_INLINE bool convertToUtf8(BufferType& buffer, const NonUtf8Type* start)
{
    auto& ToUtf8 = std::use_facet<std::codecvt<NonUtf8Type, Utf8, std::mbstate_t>>(std::locale());
    const auto* end = String::recurseToNullEnd(start);

    // Convert from UTF-16/UTF-32 to UTF-8
    std::mbstate_t state{};
    const NonUtf8Type* nextFrom = nullptr;
    Utf8* nextTo = nullptr;

    buffer.resize(ToUtf8.max_length() * (end - start), TCHAR('\0'));
    Utf8* outData = reinterpret_cast<Utf8*>(buffer.data());

    std::codecvt_base::result status = ToUtf8.out(state
        , reinterpret_cast<const NonUtf8Type*>(start), reinterpret_cast<const NonUtf8Type*>(end), nextFrom
        , outData, outData + buffer.size(), nextTo);
    buffer.resize(nextTo - outData);
    if (status != std::codecvt_base::ok)
    {
        LOG_ERROR("StringConv", "Failed to convert to AChar(UTF-8)");
        buffer.clear();
        return false;
    }
    return true;
}

template <typename BufferType, typename NonUtf8Type = BufferType::value_type>
FORCE_INLINE bool convertFromUtf8(BufferType& buffer, const AChar* start)
{
    auto& fromUtf8 = std::use_facet<std::codecvt<NonUtf8Type, Utf8, std::mbstate_t>>(std::locale());
    const auto* end = String::recurseToNullEnd(start);

    // Convert from UTF-8 to UTF-16/UTF-32
    std::mbstate_t state{};
    const Utf8* nextFrom = nullptr;
    NonUtf8Type* nextTo = nullptr;

    buffer.resize(end - start, TCHAR('\0'));
    NonUtf8Type* outData = reinterpret_cast<NonUtf8Type*>(buffer.data());
    std::codecvt_base::result status = fromUtf8.in(state
        , reinterpret_cast<const Utf8*>(start), reinterpret_cast<const Utf8*>(end), nextFrom
        , outData, outData + buffer.size(), nextTo);
    buffer.resize(nextTo - outData);
    if (status != std::codecvt_base::ok)
    {
        LOG_ERROR("StringConv", "Failed to convert from AChar(UTF-8)");
        buffer.clear();
        return false;
    }
    return true;
}

const AChar* StringConv<WChar, AChar>::convert(const WChar* start)
{
    if (!(PlatformFunctions::wcharToUtf8(str, start) || convertToUtf8(str, start)))
    {
        LOG_ERROR("StringConv", "Failed to convert from WChar(UTF-16/UTF-32) to AChar(UTF-8)");
    }
    return str.c_str();
}
const WChar* StringConv<AChar, WChar>::convert(const AChar* start)
{
    if (!(PlatformFunctions::utf8ToWChar(str, start) || convertFromUtf8(str, start)))
    {
        LOG_ERROR("StringConv", "Failed to convert from AChar(UTF-8) to WChar(UTF-16/UTF-32)");
    }
    return str.c_str();
}

const Utf16* StlStringConv<AChar, Utf16>::convert(const AChar* start)
{
    if (!convertFromUtf8(str, start))
    {
        LOG_ERROR("StringConv", "Failed to convert from AChar(UTF-8) to UTF-16");
    }
    return str.c_str();
}
const AChar* StlStringConv<Utf16, AChar>::convert(const Utf16* start)
{
    if (!convertToUtf8(str, start))
    {
        LOG_ERROR("StringConv", "Failed to convert from UTF-16 to AChar(UTF-8)");
    }
    return str.c_str();
}

const Utf32* StlStringConv<AChar, Utf32>::convert(const AChar* start)
{
    if (!convertFromUtf8(str, start))
    {
        LOG_ERROR("StringConv", "Failed to convert from AChar(UTF-8) to UTF-32");
    }
    return str.c_str();
}
const AChar* StlStringConv<Utf32, AChar>::convert(const Utf32* start)
{
    if (!convertToUtf8(str, start))
    {
        LOG_ERROR("StringConv", "Failed to convert from UTF-32 to AChar(UTF-8)");
    }
    return str.c_str();
}

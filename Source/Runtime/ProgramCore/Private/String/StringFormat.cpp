#include "String/StringFormat.h"
#include "Logger/Logger.h"
#include "Types/Platform/PlatformAssertionErrors.h"

#include <regex>

FormatArg::FormatArg(const FormatArg& arg)
    : type(arg.type)
{
    (*this) = arg;
}

FormatArg::FormatArg(FormatArg&& arg)
    : type(arg.type)
{
    (*this) = std::move(arg);
}

FormatArg& FormatArg::operator=(FormatArg&& arg)
{
    type = std::move(arg.type);
    switch (type)
    {
    case FormatArg::Bool:
        value.fundamentalVals.boolVal = std::move(arg.value.fundamentalVals.boolVal);
        break;
    case FormatArg::UInt8:
        value.fundamentalVals.uInt8Val = std::move(arg.value.fundamentalVals.uInt8Val);
        break;
    case FormatArg::UInt16:
        value.fundamentalVals.uInt16Val = std::move(arg.value.fundamentalVals.uInt16Val);
        break;
    case FormatArg::UInt32:
        value.fundamentalVals.uInt32Val = std::move(arg.value.fundamentalVals.uInt32Val);
        break;
    case FormatArg::UInt64:
        value.fundamentalVals.uInt64Val = std::move(arg.value.fundamentalVals.uInt64Val);
        break;
    case FormatArg::Int8:
        value.fundamentalVals.int8Val = std::move(arg.value.fundamentalVals.int8Val);
        break;
    case FormatArg::Int16:
        value.fundamentalVals.int16Val = std::move(arg.value.fundamentalVals.int16Val);
        break;
    case FormatArg::Int32:
        value.fundamentalVals.int32Val = std::move(arg.value.fundamentalVals.int32Val);
        break;
    case FormatArg::Int64:
        value.fundamentalVals.int64Val = std::move(arg.value.fundamentalVals.int64Val);
        break;
    case FormatArg::Float:
        value.fundamentalVals.floatVal = std::move(arg.value.fundamentalVals.floatVal);
        break;
    case FormatArg::Double:
        value.fundamentalVals.doubleVal = std::move(arg.value.fundamentalVals.doubleVal);
        break;
    case FormatArg::AsString:
        value.strVal = std::move(arg.value.strVal);
        break;
    default:
        break;
    }
    return *this;
}

FormatArg& FormatArg::operator=(const FormatArg& arg)
{
    type = arg.type;
    switch (type)
    {
    case FormatArg::Bool:
        value.fundamentalVals.boolVal = arg.value.fundamentalVals.boolVal;
        break;
    case FormatArg::UInt8:
        value.fundamentalVals.uInt8Val = arg.value.fundamentalVals.uInt8Val;
        break;
    case FormatArg::UInt16:
        value.fundamentalVals.uInt16Val = arg.value.fundamentalVals.uInt16Val;
        break;
    case FormatArg::UInt32:
        value.fundamentalVals.uInt32Val = arg.value.fundamentalVals.uInt32Val;
        break;
    case FormatArg::UInt64:
        value.fundamentalVals.uInt64Val = arg.value.fundamentalVals.uInt64Val;
        break;
    case FormatArg::Int8:
        value.fundamentalVals.int8Val = arg.value.fundamentalVals.int8Val;
        break;
    case FormatArg::Int16:
        value.fundamentalVals.int16Val = arg.value.fundamentalVals.int16Val;
        break;
    case FormatArg::Int32:
        value.fundamentalVals.int32Val = arg.value.fundamentalVals.int32Val;
        break;
    case FormatArg::Int64:
        value.fundamentalVals.int64Val = arg.value.fundamentalVals.int64Val;
        break;
    case FormatArg::Float:
        value.fundamentalVals.floatVal = arg.value.fundamentalVals.floatVal;
        break;
    case FormatArg::Double:
        value.fundamentalVals.doubleVal = arg.value.fundamentalVals.doubleVal;
        break;
    case FormatArg::AsString:
        value.strVal = arg.value.strVal;
        break;
    default:
        break;
    }

    return *this;
}

String FormatArg::toString() const
{
    switch (type)
    {
    case FormatArg::Bool:
        return (value.fundamentalVals.boolVal) ? "true" : "false";
        break;
    case FormatArg::UInt8:
        return StringFormat::format("%hhu", value.fundamentalVals.uInt8Val);
        break;
    case FormatArg::UInt16:
        return StringFormat::format("%hu", value.fundamentalVals.uInt16Val);
        break;
    case FormatArg::UInt32:
        return StringFormat::format("%u", value.fundamentalVals.uInt32Val);
        break;
    case FormatArg::UInt64:
        return StringFormat::format("%llu", value.fundamentalVals.uInt64Val);
        break;
    case FormatArg::Int8:
        return StringFormat::format("%hhd", value.fundamentalVals.int8Val);
        break;
    case FormatArg::Int16:
        return StringFormat::format("%hd", value.fundamentalVals.int16Val);
        break;
    case FormatArg::Int32:
        return StringFormat::format("%d", value.fundamentalVals.int32Val);
        break;
    case FormatArg::Int64:
        return StringFormat::format("%lld", value.fundamentalVals.int64Val);
        break;
    case FormatArg::Float:
        return StringFormat::format("%f", value.fundamentalVals.floatVal);
        break;
    case FormatArg::Double:
        return StringFormat::format("%f", value.fundamentalVals.doubleVal);
        break;
    case FormatArg::AsString:
        return value.strVal;
        break;
    default:
        break;
    }
    return "Invalid FormatArg";
}

String StringFormat::formatMustache(const String& fmt, const std::unordered_map<String, FormatArg>& formatArgs)
{
    std::vector<std::smatch> allMatches;
    try
    {
        // Scans for pattern within a line, Matches inner most {{.+}} and captures the inner name of the match
        static const std::regex searchPattern("\\{\\{([^{}]+)\\}\\}", std::regex_constants::ECMAScript);

        auto startItr = fmt.cbegin();
        std::smatch matches;
        while (std::regex_search(startItr, fmt.cend(), matches, searchPattern))
        {
            allMatches.push_back(matches);
            startItr = matches.suffix().first;
        }
    }
    catch (const std::exception& e)
    {
        Logger::error("StringFormat", "%s() : Error : %s", __func__, e.what());
        fatalAssert(false, "%s() : %s", __func__, e.what());
    }

    // Each segment starts at first index of prefix and has extend
    struct FormatSegment
    {
        uint64 prefixStartIndex;
        uint64 length;
    };
    std::vector<FormatSegment> outSegments;
    outSegments.reserve(allMatches.size());
    uint64 backSegmentLength = 0;
    uint64 outStrLength = fmt.length();

    std::unordered_map<String, String> formatStrings;
    // Below loop creates arg string for each matched mustache from formatArgs
    // Then creates post replacement offsets for each prefix from each match 
    // and also as side effect creates post replace suffix offset for last match as well
    for (const std::smatch& match : allMatches)
    {
        const std::ssub_match& submatch = match[match.size() - 1];
        const std::ssub_match& wholesubmatch = match[0];

        String argName = submatch.str();
        auto formatArgItr = formatArgs.find(argName);
        if (formatArgItr == formatArgs.cend()) // Match's FormatArg not found so we skip and wont change final string size here
        {
            Logger::warn("StringFormat", "%s() : Format Arg not found for Arg Name %s", __func__, argName);
            outSegments.emplace_back(
                FormatSegment
                { 
                    backSegmentLength
                    , uint64(match.prefix().length() + wholesubmatch.length()) 
                });
        }
        else
        {
            uint64 replacementLength = 0;
            auto formatStrItr = formatStrings.find(argName);
            // If we already generated string for this format arg name we do not need to format it again.
            if (formatStrItr == formatStrings.end())
            {
                String replacementStr = formatArgItr->second.toString();
                formatStrings[argName] = replacementStr;
                replacementLength = replacementStr.length();
            }
            else
            {
                replacementLength = formatStrItr->second.length();
            }
            // increase or decrease the size of final string based on each match replacement difference with original string
            outStrLength += (replacementLength - wholesubmatch.length());
            outSegments.emplace_back(
                FormatSegment
                {
                    backSegmentLength
                    , match.prefix().length() + replacementLength
                });
        }
        backSegmentLength = outSegments.back().prefixStartIndex + outSegments.back().length;
    }

    // Now copy all strings to final string, All segments are non overlapping and can be parallel processed
    String outputStr;
    outputStr.resize(outStrLength);
    for (uint32 i = 0; i < allMatches.size(); ++i)
    {
        const std::smatch& match = allMatches[i];
        const FormatSegment& fmtSegment = outSegments[i];

        const std::ssub_match& wholesubmatch = match[0];
        String argName = match[match.size() - 1].str();
        auto formatStrItr = formatStrings.find(argName);
        
        // Append prefix to output
        outputStr.replace(
            outputStr.cbegin() + fmtSegment.prefixStartIndex
            , outputStr.cbegin() + fmtSegment.prefixStartIndex + match.prefix().length()
            , match.prefix().first, match.prefix().second);
        if (formatStrItr == formatStrings.cend())
        {
            // Append match string itself as we cannot find any format for this
            outputStr.replace(
                outputStr.cbegin() + fmtSegment.prefixStartIndex + match.prefix().length()
                , outputStr.cbegin() + fmtSegment.prefixStartIndex + fmtSegment.length
                , wholesubmatch.first, wholesubmatch.second);
        }
        else
        {
            // Append replacement
            outputStr.replace(
                fmtSegment.prefixStartIndex + match.prefix().length()
                , formatStrItr->second.length()
                , formatStrItr->second);
        }
    }
    // Now append final suffix
    outputStr.replace(
        outSegments.back().prefixStartIndex + outSegments.back().length
        , allMatches.back().suffix().length()
        , &(*allMatches.back().suffix().first)
        , allMatches.back().suffix().length());

    return outputStr;
}

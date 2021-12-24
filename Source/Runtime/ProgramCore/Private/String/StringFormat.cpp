#include "String/StringFormat.h"
#include "String/MustacheFormatString.h"
#include "Logger/Logger.h"
#include "Types/Platform/PlatformAssertionErrors.h"

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
        value.fundamentalVals.uint8Val = std::move(arg.value.fundamentalVals.uint8Val);
        break;
    case FormatArg::UInt16:
        value.fundamentalVals.uint16Val = std::move(arg.value.fundamentalVals.uint16Val);
        break;
    case FormatArg::UInt32:
        value.fundamentalVals.uint32Val = std::move(arg.value.fundamentalVals.uint32Val);
        break;
    case FormatArg::UInt64:
        value.fundamentalVals.uint64Val = std::move(arg.value.fundamentalVals.uint64Val);
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
    case FormatArg::Getter:
        value.argGetter = std::move(arg.value.argGetter);
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
        value.fundamentalVals.uint8Val = arg.value.fundamentalVals.uint8Val;
        break;
    case FormatArg::UInt16:
        value.fundamentalVals.uint16Val = arg.value.fundamentalVals.uint16Val;
        break;
    case FormatArg::UInt32:
        value.fundamentalVals.uint32Val = arg.value.fundamentalVals.uint32Val;
        break;
    case FormatArg::UInt64:
        value.fundamentalVals.uint64Val = arg.value.fundamentalVals.uint64Val;
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
    case FormatArg::Getter:
        value.argGetter = arg.value.argGetter;
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
        return StringFormat::format("%hhu", value.fundamentalVals.uint8Val);
        break;
    case FormatArg::UInt16:
        return StringFormat::format("%hu", value.fundamentalVals.uint16Val);
        break;
    case FormatArg::UInt32:
        return StringFormat::format("%u", value.fundamentalVals.uint32Val);
        break;
    case FormatArg::UInt64:
        return StringFormat::format("%llu", value.fundamentalVals.uint64Val);
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
    case FormatArg::Getter:
        return value.argGetter.invoke();
    case FormatArg::AsString:
        return value.strVal;
        break;
    default:
        break;
    }
    return "Invalid FormatArg";
}

FormatArg::operator bool() const
{
    switch (type)
    {
    case FormatArg::Bool:
        return value.fundamentalVals.boolVal;
        break;
    // #TODO(Jeslas) : Check if for uint8 sets all higher bits 0 in uint64
    case FormatArg::UInt8:
    case FormatArg::UInt16:
    case FormatArg::UInt32:
    case FormatArg::UInt64:
        return (value.fundamentalVals.uint64Val != 0);
        break;
    case FormatArg::Int8:
        return (value.fundamentalVals.int8Val != 0);
        break;
    case FormatArg::Int16:
        return (value.fundamentalVals.int16Val != 0);
        break;
    case FormatArg::Int32:
        return (value.fundamentalVals.int32Val != 0);
        break;
    case FormatArg::Int64:
        return (value.fundamentalVals.int64Val) != 0;
        break;
    case FormatArg::Float:
        return (value.fundamentalVals.floatVal) != 0;
        break;
    case FormatArg::Double:
        return (value.fundamentalVals.doubleVal) != 0;
        break;
    case FormatArg::Getter:
        return value.argGetter.isBound() && !value.argGetter.invoke().empty();
        break;
    case FormatArg::AsString:
        return !value.strVal.empty();
        break;
    default:
        break;
    }
    return false;
}

String StringFormat::formatMustache(const String& fmt, const FormatArgsMap& formatArgs)
{
    MustacheStringFormatter formatter(fmt);
    return formatter.formatBasic(formatArgs);
}

//////////////////////////////////////////////////////////////////////////
/// MustacheFormatString
//////////////////////////////////////////////////////////////////////////

MustacheStringFormatter::MustacheStringFormatter(const String& fmt)
    : fmtStr(fmt)
    , allMatches()
{
    // Scans for pattern within a line, Matches inner most {{.+}} and captures the inner name of the match
    static const std::regex searchPattern("\\{\\{([^{}]+)\\}\\}", std::regex_constants::ECMAScript);

    auto startItr = fmtStr.cbegin();
    std::smatch matches;
    while (std::regex_search(startItr, fmtStr.cend(), matches, searchPattern))
    {
        allMatches.push_back(matches);
        startItr = matches.suffix().first;
    }

    std::vector<std::pair<String, uint32>> sectAndIdxStack;
    for (uint32 i = 0; i < allMatches.size(); ++i)
    {
        const std::smatch& match = allMatches[i];
        String matchStr{ match[match.size() - 1].str() };
        String argName = matchStr;
        removeMustachePrefix(argName);

        if (isASection(matchStr))
        {
            sectAndIdxStack.push_back({ argName, uint32(sections.size()) });
            Section& section = sections.emplace_back();
            section.sectionStartIdx = i;
        }
        else if (isSectionClose(matchStr))
        {
            fatalAssert(sectAndIdxStack.back().first == argName, "%s() : Section tag %s is not closed", __func__, sectAndIdxStack.back().first);
            sections[sectAndIdxStack.back().second].childCount = uint32(sections.size() - (sectAndIdxStack.back().second + 1));
            sections[sectAndIdxStack.back().second].sectionEndIdx = i;
            sectAndIdxStack.pop_back();
        }
    }
}

FORCE_INLINE void MustacheStringFormatter::removeMustachePrefix(String& tagName) const
{
    // Match and replace first char
    static const std::regex searchPattern("^[#^!>/]{1}", std::regex_constants::ECMAScript);

    tagName = std::regex_replace(tagName, searchPattern, "");
    tagName.trim();
}

String MustacheStringFormatter::formatBasic(const FormatArgsMap& formatArgs) const
{
    // Each segment starts at first index of prefix and has extend
    struct FormatSegment
    {
        uint64 prefixStartIndex;
        uint64 length;
    };
    std::vector<FormatSegment> outSegments;
    outSegments.reserve(allMatches.size());
    uint64 backSegmentLength = 0;
    uint64 outStrLength = fmtStr.length();

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
        if (isAComment(argName))
        {
            outSegments.emplace_back(
                FormatSegment
                {
                    backSegmentLength
                    , 0
                });
            outStrLength -= wholesubmatch.length();
        }
        else if (formatArgItr == formatArgs.cend()) // Match's FormatArg not found so we skip and wont change final string size here
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
        if (isAComment(argName))
        {
            continue;
        }

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

void MustacheStringFormatter::renderSection(std::ostringstream& outStr, uint32 sectionIdx, const FormatArgsMap& args
    , const std::unordered_map<String, std::vector<FormatArgsMap>>& sectionArgs
    , const std::unordered_map<String, MustacheSectionFormatter>& sectionFormatters
    , const std::unordered_map<String, MustacheStringFormatter>& partials) const
{
    const Section& section = sections[sectionIdx];

    const std::smatch& match = allMatches[section.sectionStartIdx];
    const std::ssub_match& submatch = match[match.size() - 1];
    const std::ssub_match& wholesubmatch = match[0];

    String matchStr = submatch.str();
    String argName = matchStr;
    removeMustachePrefix(argName);

    std::unordered_map<String, MustacheSectionFormatter>::const_iterator sectionFormatterItr = sectionFormatters.find(argName);
    std::unordered_map<String, std::vector<FormatArgsMap>>::const_iterator additionalArgsItr = sectionArgs.find(argName);
    FormatArgsMap::const_iterator argItr = args.find(argName);

    // If not condition enabled then all condition must fail to render
    if (isANotSection(matchStr))
    {
        if (sectionFormatterItr == sectionFormatters.cend()
            && (additionalArgsItr == sectionArgs.cend() || additionalArgsItr->second.empty())
            && (argItr == args.cend() || !argItr->second))
        {
            // Render all inner tags
            for (uint32 matchIdx = section.sectionStartIdx + 1; matchIdx < section.sectionEndIdx;)
            {
                matchIdx = renderTag(outStr, matchIdx, args, sectionArgs, sectionFormatters, partials);
            }
        }
    }
    else
    {
        if (sectionFormatterItr != sectionFormatters.cend())
        {
            if (sectionFormatterItr->second.isBound())
            {
                // Start match's suffix start itr to end match's prefix end itr gives the inner unformatted string
                MustacheStringFormatter innerFormatter{ String(match.suffix().first, allMatches[section.sectionEndIdx].prefix().second) };
                MustacheContext context
                {
                    .args = &args,
                    .sectionArgs = &sectionArgs,
                    .sectionFormatters = &sectionFormatters,
                    .partials = &partials
                };

                outStr << sectionFormatterItr->second.invoke(innerFormatter, context);
            }
            else
            {
                // If found formatter and still not bound then we remove this section, Even if negate condition unbound cannot be executed
                Logger::error("MustacheStringFormatter", "%s() : Section formatter function found for section {{%s}}, but it is unbound!", __func__, matchStr);
            }
        }
        // If sectionArgs are found and they are valid then we render the tags inside section
        else if (additionalArgsItr != sectionArgs.cend() && !additionalArgsItr->second.empty())
        {
            for (const FormatArgsMap& additionalArgs : additionalArgsItr->second)
            {
                // First additionalArgs to allow overriding default args
                FormatArgsMap newArgs(additionalArgs);
                newArgs.insert(args.cbegin(), args.cend());

                // Render all inner tags for each section args list
                for (uint32 matchIdx = section.sectionStartIdx + 1; matchIdx < section.sectionEndIdx;)
                {
                    matchIdx = renderTag(outStr, matchIdx, newArgs, sectionArgs, sectionFormatters, partials);
                }
            }
        }
        // If section representing arg is true, Gives valid result and can render output
        else if (argItr != args.cend() && argItr->second)
        {
            // Render all inner tags
            for (uint32 matchIdx = section.sectionStartIdx + 1; matchIdx < section.sectionEndIdx;)
            {
                matchIdx = renderTag(outStr, matchIdx, args, sectionArgs, sectionFormatters, partials);
            }
        }
    }
}

uint32 MustacheStringFormatter::renderTag(std::ostringstream& outStr, uint32 matchIdx, const FormatArgsMap& args
    , const std::unordered_map<String, std::vector<FormatArgsMap>>& sectionArgs
    , const std::unordered_map<String, MustacheSectionFormatter>& sectionFormatters
    , const std::unordered_map<String, MustacheStringFormatter>& partials) const
{
    // Append match's prefix
    outStr << std::string_view(allMatches[matchIdx].prefix().first, allMatches[matchIdx].prefix().second);

    const std::smatch& match = allMatches[matchIdx];
    const std::ssub_match& submatch = match[match.size() - 1];
    const std::ssub_match& wholesubmatch = match[0];

    String matchStr = submatch.str();
    String argName = matchStr;
    removeMustachePrefix(argName);

    if (isAPartial(matchStr))
    {
        auto partialItr = partials.find(argName);
        if (partialItr == partials.cend())
        {
            Logger::error("MustacheStringFormatter", "%s() : Could not find any partial for partial tag {{%s}}", __func__, matchStr);
        }
        else
        {
            outStr << partialItr->second.render(args, sectionArgs, sectionFormatters, partials);
        }
    }
    else if (isASection(matchStr))
    {
        auto itr = std::find_if(sections.cbegin(), sections.cend(), [matchIdx](const Section& section) { return matchIdx == section.sectionStartIdx; });
        fatalAssert(itr != sections.cend(), "%s() : Section %s not found in sections list", __func__, argName);
        uint32 sectionIdx = std::distance(sections.cbegin(), itr);
        renderSection(outStr, sectionIdx, args, sectionArgs, sectionFormatters, partials);

        return itr->sectionEndIdx + 1;
    }
    else if (!isAComment(matchStr))
    {
        auto arg = args.find(argName);
        if (arg == args.end())
        {
            Logger::error("MustacheStringFormatter", "%s() : Could not find format arg for tag {{%s}}", __func__, matchStr);
        }
        else
        {
            outStr << arg->second.toString();
        }
    }
    return matchIdx + 1;
}

String MustacheStringFormatter::render(const FormatArgsMap& args, const std::unordered_map<String, std::vector<FormatArgsMap>>& sectionArgs
    , const std::unordered_map<String, MustacheSectionFormatter>& sectionFormatters, const std::unordered_map<String, MustacheStringFormatter>& partials) const
{
    std::ostringstream outputStr;
    for (uint32 matchIdx = 0; matchIdx < allMatches.size();)
    {
        matchIdx = renderTag(outputStr, matchIdx, args, sectionArgs, sectionFormatters, partials);
    }
    // Now append final suffix
    outputStr << std::string_view(allMatches.back().suffix().first, allMatches.back().suffix().second);
    return outputStr.str();
}


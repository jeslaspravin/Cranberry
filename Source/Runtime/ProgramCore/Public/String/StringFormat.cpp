/*!
 * \file StringFormat.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2024
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "String/StringFormat.h"
#include "Types/Platform/PlatformAssertionErrors.h"
#include "Logger/Logger.h"
#include "String/MustacheFormatString.h"

//////////////////////////////////////////////////////////////////////////
/// MustacheFormatString
//////////////////////////////////////////////////////////////////////////

MustacheFormatArg::MustacheFormatArg(const MustacheFormatArg &arg) noexcept
    : type(arg.type)
{
    (*this) = arg;
}

MustacheFormatArg::MustacheFormatArg(MustacheFormatArg &&arg) noexcept
    : type(arg.type)
{
    (*this) = std::move(arg);
}

MustacheFormatArg &MustacheFormatArg::operator= (MustacheFormatArg &&arg) noexcept
{
    type = std::move(arg.type);
    switch (type)
    {
    case MustacheFormatArg::Bool:
        value.fundamentalVals.boolVal = std::move(arg.value.fundamentalVals.boolVal);
        break;
    case MustacheFormatArg::UInt8:
        value.fundamentalVals.uint8Val = std::move(arg.value.fundamentalVals.uint8Val);
        break;
    case MustacheFormatArg::UInt16:
        value.fundamentalVals.uint16Val = std::move(arg.value.fundamentalVals.uint16Val);
        break;
    case MustacheFormatArg::UInt32:
        value.fundamentalVals.uint32Val = std::move(arg.value.fundamentalVals.uint32Val);
        break;
    case MustacheFormatArg::UInt64:
        value.fundamentalVals.uint64Val = std::move(arg.value.fundamentalVals.uint64Val);
        break;
    case MustacheFormatArg::Int8:
        value.fundamentalVals.int8Val = std::move(arg.value.fundamentalVals.int8Val);
        break;
    case MustacheFormatArg::Int16:
        value.fundamentalVals.int16Val = std::move(arg.value.fundamentalVals.int16Val);
        break;
    case MustacheFormatArg::Int32:
        value.fundamentalVals.int32Val = std::move(arg.value.fundamentalVals.int32Val);
        break;
    case MustacheFormatArg::Int64:
        value.fundamentalVals.int64Val = std::move(arg.value.fundamentalVals.int64Val);
        break;
    case MustacheFormatArg::Float:
        value.fundamentalVals.floatVal = std::move(arg.value.fundamentalVals.floatVal);
        break;
    case MustacheFormatArg::Double:
        value.fundamentalVals.doubleVal = std::move(arg.value.fundamentalVals.doubleVal);
        break;
    case MustacheFormatArg::Getter:
        value.argGetter = std::move(arg.value.argGetter);
        break;
    case MustacheFormatArg::AsString:
        value.strVal = std::move(arg.value.strVal);
        break;
    default:
        break;
    }
    return *this;
}

MustacheFormatArg &MustacheFormatArg::operator= (const MustacheFormatArg &arg) noexcept
{
    type = arg.type;
    switch (type)
    {
    case MustacheFormatArg::Bool:
        value.fundamentalVals.boolVal = arg.value.fundamentalVals.boolVal;
        break;
    case MustacheFormatArg::UInt8:
        value.fundamentalVals.uint8Val = arg.value.fundamentalVals.uint8Val;
        break;
    case MustacheFormatArg::UInt16:
        value.fundamentalVals.uint16Val = arg.value.fundamentalVals.uint16Val;
        break;
    case MustacheFormatArg::UInt32:
        value.fundamentalVals.uint32Val = arg.value.fundamentalVals.uint32Val;
        break;
    case MustacheFormatArg::UInt64:
        value.fundamentalVals.uint64Val = arg.value.fundamentalVals.uint64Val;
        break;
    case MustacheFormatArg::Int8:
        value.fundamentalVals.int8Val = arg.value.fundamentalVals.int8Val;
        break;
    case MustacheFormatArg::Int16:
        value.fundamentalVals.int16Val = arg.value.fundamentalVals.int16Val;
        break;
    case MustacheFormatArg::Int32:
        value.fundamentalVals.int32Val = arg.value.fundamentalVals.int32Val;
        break;
    case MustacheFormatArg::Int64:
        value.fundamentalVals.int64Val = arg.value.fundamentalVals.int64Val;
        break;
    case MustacheFormatArg::Float:
        value.fundamentalVals.floatVal = arg.value.fundamentalVals.floatVal;
        break;
    case MustacheFormatArg::Double:
        value.fundamentalVals.doubleVal = arg.value.fundamentalVals.doubleVal;
        break;
    case MustacheFormatArg::Getter:
        value.argGetter = arg.value.argGetter;
        break;
    case MustacheFormatArg::AsString:
        value.strVal = arg.value.strVal;
        break;
    default:
        break;
    }

    return *this;
}

#define FORMAT_FUNDAMENTALS(VarName) STR_FORMAT("{}", value.fundamentalVals.##VarName)
String MustacheFormatArg::toString() const noexcept
{
    switch (type)
    {
    case MustacheFormatArg::Bool:
        return (value.fundamentalVals.boolVal) ? TCHAR("true") : TCHAR("false");
        break;
    case MustacheFormatArg::UInt8:
        return FORMAT_FUNDAMENTALS(uint8Val);
        break;
    case MustacheFormatArg::UInt16:
        return FORMAT_FUNDAMENTALS(uint16Val);
        break;
    case MustacheFormatArg::UInt32:
        return FORMAT_FUNDAMENTALS(uint32Val);
        break;
    case MustacheFormatArg::UInt64:
        return FORMAT_FUNDAMENTALS(uint64Val);
        break;
    case MustacheFormatArg::Int8:
        return FORMAT_FUNDAMENTALS(int8Val);
        break;
    case MustacheFormatArg::Int16:
        return FORMAT_FUNDAMENTALS(int16Val);
        break;
    case MustacheFormatArg::Int32:
        return FORMAT_FUNDAMENTALS(int32Val);
        break;
    case MustacheFormatArg::Int64:
        return FORMAT_FUNDAMENTALS(int64Val);
        break;
    case MustacheFormatArg::Float:
        return FORMAT_FUNDAMENTALS(floatVal);
        break;
    case MustacheFormatArg::Double:
        return FORMAT_FUNDAMENTALS(doubleVal);
        break;
    case MustacheFormatArg::Getter:
        return value.argGetter.invoke();
    case MustacheFormatArg::AsString:
        return value.strVal;
        break;
    default:
        break;
    }
    return TCHAR("Invalid FormatArg");
}
#undef FORMAT_FUNDAMENTALS

MustacheFormatArg::operator bool () const noexcept
{
    switch (type)
    {
    case MustacheFormatArg::Bool:
        return value.fundamentalVals.boolVal;
        break;
    case MustacheFormatArg::UInt8:
        return (value.fundamentalVals.uint8Val != 0);
    case MustacheFormatArg::UInt16:
        return (value.fundamentalVals.uint16Val != 0);
    case MustacheFormatArg::UInt32:
        return (value.fundamentalVals.uint32Val != 0);
    case MustacheFormatArg::UInt64:
        return (value.fundamentalVals.uint64Val != 0);
        break;
    case MustacheFormatArg::Int8:
        return (value.fundamentalVals.int8Val != 0);
        break;
    case MustacheFormatArg::Int16:
        return (value.fundamentalVals.int16Val != 0);
        break;
    case MustacheFormatArg::Int32:
        return (value.fundamentalVals.int32Val != 0);
        break;
    case MustacheFormatArg::Int64:
        return (value.fundamentalVals.int64Val) != 0;
        break;
    case MustacheFormatArg::Float:
        return (value.fundamentalVals.floatVal) != 0;
        break;
    case MustacheFormatArg::Double:
        return (value.fundamentalVals.doubleVal) != 0;
        break;
    case MustacheFormatArg::Getter:
        return value.argGetter.isBound() && !value.argGetter.invoke().empty();
        break;
    case MustacheFormatArg::AsString:
        return !value.strVal.empty();
        break;
    default:
        break;
    }
    return false;
}

MustacheStringFormatter::MustacheStringFormatter(const String &fmt) noexcept
    : fmtStr(fmt)
    , allMatches()
{
    parseFmtStr();
}

void MustacheStringFormatter::parseFmtStr() noexcept
{
    // Scans for pattern within a line, Matches inner most {{.+}} and captures the inner name of the
    // match
    static const StringRegex SEARCH_PATTERN(TCHAR("\\{\\{([^{}]+)\\}\\}"), std::regex_constants::ECMAScript);
    allMatches.clear();
    sections.clear();

    auto startItr = fmtStr.cbegin();
    StringMatch matches;
    while (std::regex_search(startItr, fmtStr.cend(), matches, SEARCH_PATTERN))
    {
        allMatches.push_back(matches);
        startItr = matches.suffix().first;
    }

    std::vector<std::pair<String, uint32>> sectAndIdxStack;
    for (uint32 i = 0; i < allMatches.size(); ++i)
    {
        const StringMatch &match = allMatches[i];
        String matchStr{ match[match.size() - 1].str() };
        String argName = matchStr;
        removeMustachePrefix(argName);

        if (isASection(matchStr))
        {
            sectAndIdxStack.push_back({ argName, uint32(sections.size()) });
            Section &section = sections.emplace_back();
            section.sectionStartIdx = i;
        }
        else if (isSectionClose(matchStr))
        {
            fatalAssertf(sectAndIdxStack.back().first == argName, "Section tag {} is not closed", sectAndIdxStack.back().first);
            sections[sectAndIdxStack.back().second].childCount = uint32(sections.size() - (sectAndIdxStack.back().second + 1));
            sections[sectAndIdxStack.back().second].sectionEndIdx = i;
            sectAndIdxStack.pop_back();
        }
    }
}

FORCE_INLINE void MustacheStringFormatter::removeMustachePrefix(String &tagName) const noexcept
{
    // Match and replace first char
    static const StringRegex SEARCH_PATTERN(TCHAR("^[#^!>/]{1}"), std::regex_constants::ECMAScript);

    tagName = std::regex_replace(tagName, SEARCH_PATTERN, TCHAR(""));
    tagName.trim();
}

String MustacheStringFormatter::formatBasic(const FormatArgsMap &formatArgs) const noexcept
{
    /* If no matches then return format string itself */
    if (allMatches.empty())
    {
        return fmtStr;
    }

    /* Each segment starts at first index of prefix and has extend */
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
    /* Below loop creates arg string for each matched mustache from formatArgs
     * Then creates post replacement offsets for each prefix from each match
     * and also as side effect creates post replace suffix offset for last match as well */
    for (const StringMatch &match : allMatches)
    {
        const StringSubmatch &submatch = match[match.size() - 1];
        const StringSubmatch &wholesubmatch = match[0];

        String argName = submatch.str();
        auto formatArgItr = formatArgs.find(argName);
        if (isAComment(argName))
        {
            outSegments.emplace_back(FormatSegment{ backSegmentLength, 0 });
            outStrLength -= wholesubmatch.length();
        }
        else if (formatArgItr == formatArgs.cend()) // Match's FormatArg not found so we skip and
                                                    // wont change final string size here
        {
            LOG_WARN("StringFormat", "Format Arg not found for Arg Name {}", argName);
            outSegments.emplace_back(FormatSegment{ backSegmentLength, uint64(match.prefix().length() + wholesubmatch.length()) });
        }
        else
        {
            uint64 replacementLength = 0;
            auto formatStrItr = formatStrings.find(argName);
            // If we already generated string for this format arg name we do not need to format
            // it again.
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
            // increase or decrease the size of final string based on each match replacement
            // difference with original string
            outStrLength += (replacementLength - wholesubmatch.length());
            outSegments.emplace_back(FormatSegment{ backSegmentLength, match.prefix().length() + replacementLength });
        }
        backSegmentLength = outSegments.back().prefixStartIndex + outSegments.back().length;
    }

    /* Now copy all strings to final string, All segments are non overlapping and can be parallel processed */
    String outputStr;
    outputStr.resize(outStrLength);
    for (uint32 i = 0; i < allMatches.size(); ++i)
    {
        const StringMatch &match = allMatches[i];
        const FormatSegment &fmtSegment = outSegments[i];

        const StringSubmatch &wholesubmatch = match[0];
        String argName = match[match.size() - 1].str();
        if (isAComment(argName))
        {
            continue;
        }

        auto formatStrItr = formatStrings.find(argName);
        // Append prefix to output
        outputStr.replace(
            outputStr.cbegin() + fmtSegment.prefixStartIndex, outputStr.cbegin() + fmtSegment.prefixStartIndex + match.prefix().length(),
            match.prefix().first, match.prefix().second
        );

        if (formatStrItr == formatStrings.cend())
        {
            // Append match string itself as we cannot find any format for this
            outputStr.replace(
                outputStr.cbegin() + fmtSegment.prefixStartIndex + match.prefix().length(),
                outputStr.cbegin() + fmtSegment.prefixStartIndex + fmtSegment.length, wholesubmatch.first, wholesubmatch.second
            );
        }
        else
        {
            // Append replacement
            outputStr.replace(fmtSegment.prefixStartIndex + match.prefix().length(), formatStrItr->second.length(), formatStrItr->second);
        }
    }
    // Now append final suffix
    outputStr.replace(
        outSegments.back().prefixStartIndex + outSegments.back().length, allMatches.back().suffix().length(),
        &(*allMatches.back().suffix().first), allMatches.back().suffix().length()
    );

    return outputStr;
}

FORCE_INLINE void MustacheStringFormatter::renderSectionInner(
    OStringStream &outStr, const Section &section, const MustacheContext &context,
    const std::unordered_map<String, MustacheStringFormatter> &partials
) const noexcept
{
    // Render all inner tags
    for (uint32 matchIdx = section.sectionStartIdx + 1; matchIdx < section.sectionEndIdx;)
    {
        matchIdx = renderTag(outStr, matchIdx, context, partials);
    }
    // Append any thing before the sectionEndIdx
    outStr << StringView(allMatches[section.sectionEndIdx].prefix().first, allMatches[section.sectionEndIdx].prefix().second);
}

void MustacheStringFormatter::renderSection(
    OStringStream &outStr, uint32 sectionIdx, const MustacheContext &context,
    const std::unordered_map<String, MustacheStringFormatter> &partials
) const noexcept
{
    const Section &section = sections[sectionIdx];

    const StringMatch &match = allMatches[section.sectionStartIdx];
    // In {{abc}}, match[0] will be {{abc}} match[size - 1] will be abc
    const StringSubmatch &submatch = match[match.size() - 1];

    String matchStr = submatch.str();
    String argName = matchStr;
    removeMustachePrefix(argName);

    std::unordered_map<String, MustacheSectionFormatter>::const_iterator sectionFormatterItr = context.sectionFormatters.find(argName);
    std::unordered_map<String, std::vector<MustacheContext>>::const_iterator additionalContextsItr = context.sectionContexts.find(argName);
    FormatArgsMap::const_iterator argItr = context.args.find(argName);

    // If not condition enabled then all condition must fail to render
    if (isANotSection(matchStr))
    {
        if (sectionFormatterItr == context.sectionFormatters.cend()
            && (additionalContextsItr == context.sectionContexts.cend() || additionalContextsItr->second.empty())
            && (argItr == context.args.cend() || !argItr->second))
        {
            renderSectionInner(outStr, section, context, partials);
        }
    }
    else
    {
        if (sectionFormatterItr != context.sectionFormatters.cend())
        {
            if (sectionFormatterItr->second.isBound())
            {
                // Start match's suffix start itr to end match's prefix end itr gives the
                // inner unformatted string
                MustacheStringFormatter innerFormatter{ String(match.suffix().first, allMatches[section.sectionEndIdx].prefix().second) };

                outStr << sectionFormatterItr->second.invoke(innerFormatter, context, partials);
            }
            else
            {
                // If found formatter and still not bound then we remove this section, Even
                // if negate condition unbound cannot be executed
                LOG_ERROR(
                    "MustacheStringFormatter",
                    "Section formatter function found for section {{{}}}, but it is "
                    "unbound!",
                    matchStr
                );
            }
        }
        // If sectionArgs are found and they are valid then we render the tags inside section
        else if (additionalContextsItr != context.sectionContexts.cend() && !additionalContextsItr->second.empty())
        {
            String indexArg = StringFormat::printf(INDEX_FMT, 0);
            uint32 idx = 1;
            while (context.args.contains(indexArg))
            {
                indexArg = StringFormat::printf(INDEX_FMT, idx);
                idx++;
            }
            idx = 0;
            for (const MustacheContext &additionalContext : additionalContextsItr->second)
            {
                // First additionalArgs to allow overriding default args
                MustacheContext newContext(additionalContext);
                newContext.args.insert(context.args.cbegin(), context.args.cend());
                newContext.sectionContexts.insert(context.sectionContexts.cbegin(), context.sectionContexts.cend());
                newContext.sectionFormatters.insert(context.sectionFormatters.cbegin(), context.sectionFormatters.cend());

                newContext.args[indexArg] = idx;

                // Render all inner tags for each section contexts
                renderSectionInner(outStr, section, newContext, partials);
                idx++;
            }
        }
        // If section representing arg is true, Gives valid result and can render output
        else if (argItr != context.args.cend() && argItr->second)
        {
            renderSectionInner(outStr, section, context, partials);
        }
    }
}

uint32 MustacheStringFormatter::renderTag(
    OStringStream &outStr, uint32 matchIdx, const MustacheContext &context, const std::unordered_map<String, MustacheStringFormatter> &partials
) const noexcept
{
    // Append match's prefix
    outStr << StringView(allMatches[matchIdx].prefix().first, allMatches[matchIdx].prefix().second);

    const StringMatch &match = allMatches[matchIdx];
    // In {{abc}}, match[0] will be {{abc}} match[size - 1] will be abc
    const StringSubmatch &submatch = match[match.size() - 1];

    String matchStr = submatch.str();
    String argName = matchStr;
    removeMustachePrefix(argName);

    if (isAPartial(matchStr))
    {
        auto partialItr = partials.find(argName);
        if (partialItr == partials.cend())
        {
            LOG_ERROR("MustacheStringFormatter", "Could not find any partial for partial tag {{{}}}", matchStr);
        }
        else
        {
            outStr << partialItr->second.render(context, partials);
        }
    }
    else if (isASection(matchStr))
    {
        auto itr = std::find_if(
            sections.cbegin(), sections.cend(),
            [matchIdx](const Section &section)
            {
                return matchIdx == section.sectionStartIdx;
            }
        );
        fatalAssertf(itr != sections.cend(), "Section {} not found in sections list", argName);
        uint32 sectionIdx = uint32(std::distance(sections.cbegin(), itr));
        renderSection(outStr, sectionIdx, context, partials);

        return itr->sectionEndIdx + 1;
    }
    else if (!isAComment(matchStr))
    {
        auto arg = context.args.find(argName);
        if (arg == context.args.end())
        {
            LOG_ERROR("MustacheStringFormatter", "Could not find format arg for tag {{{}}}", matchStr);
        }
        else
        {
            outStr << arg->second.toString();
        }
    }
    return matchIdx + 1;
}

String MustacheStringFormatter::render(const MustacheContext &context, const std::unordered_map<String, MustacheStringFormatter> &partials)
    const noexcept
{
    /* If no matches then return format string itself */
    if (allMatches.empty())
    {
        return fmtStr;
    }
    OStringStream outputStr;
    for (uint32 matchIdx = 0; matchIdx < allMatches.size();)
    {
        matchIdx = renderTag(outputStr, matchIdx, context, partials);
    }
    // Now append final suffix
    outputStr << StringView(allMatches.back().suffix().first, allMatches.back().suffix().second);
    return outputStr.str();
}

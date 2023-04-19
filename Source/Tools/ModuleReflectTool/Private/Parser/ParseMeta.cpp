/*!
 * \file ParseMeta.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "Parser/ParserHelper.h"
#include "Property/PropertyHelper.h"
#include "Property/PropertyMetaFlags.inl"

#include <unordered_set>

#define META_FLAG_ENTRY_FIRST(Flag) TCHAR(#Flag)
#define META_FLAG_ENTRY(Flag) , TCHAR(#Flag)

bool ParserHelper::parseMeta(std::vector<String> &possibleFlags, std::vector<ParsedMetaData> &metaData, const String &annotatedStr)
{
    bool bSuccess = true;
    const TChar *ch = annotatedStr.c_str();
    const TChar *start = ch;
    while (*ch != '\0')
    {
        switch (*ch)
        {
        case ',':
        {
            String possibleMetaFlag{ start, SizeT(ch - start) };
            possibleMetaFlag.trim();
            if (!possibleMetaFlag.empty())
            {
                possibleFlags.emplace_back(std::move(possibleMetaFlag));
            }
            start = ch + 1;
            break;
        }
        case '{':
        case '(':
        {
            // Inside a block
            int32 nestingCount = 1;
            const TChar *argsCh = ch + 1;
            while (*argsCh != '\0')
            {
                if (*argsCh == '{' || *argsCh == '(')
                {
                    nestingCount++;
                }
                else if (*argsCh == '}' || *argsCh == ')')
                {
                    nestingCount--;
                    if (nestingCount == 0)
                    {
                        break;
                    }
                }
                argsCh++;
            }

            if (nestingCount == 0)
            {
                ParsedMetaData &metaDataInfo = metaData.emplace_back(StringView(start, ch), StringView(ch + 1, argsCh));
                metaDataInfo.metaType.trim();
                metaDataInfo.ctorArgs.trim();
                ch = argsCh;
                start = ch + 1;
            }
            else
            {
                LOG_ERROR(
                    "ParserHelper", "MetaData block not properly terminated - {}\"{}...\"", StringView(start, ch), StringView(ch, argsCh)
                );
                bSuccess = false;
            }

            break;
        }
        case '}':
        case ')':
            LOG_ERROR("ParserHelper", "Inappropriate closing braces at - {}\"{}\"{}", StringView(start, ch), *ch, StringView(ch + 1));
            bSuccess = false;
            break;
        default:
            break;
        }
        ch++;
    }
    
    String possibleMetaFlag{ start };
    possibleMetaFlag.trim();
    if (!possibleMetaFlag.empty())
    {
        possibleFlags.emplace_back(std::move(possibleMetaFlag));
    }

    return bSuccess;
}

bool ParserHelper::parseClassMeta(
    std::vector<String> &metaFlags, std::vector<ParsedMetaData> &metaData, std::vector<String> &buildFlags, const String &annotatedStr
)
{
    static const std::unordered_set<String> TYPE_META_FLAGS{
        FOR_EACH_CLASS_META_FLAGS_UNIQUE_FIRST_LAST(META_FLAG_ENTRY_FIRST, META_FLAG_ENTRY, META_FLAG_ENTRY)
    };

    std::vector<String> flags;
    bool bSuccess = parseMeta(flags, metaData, annotatedStr);
    if (!bSuccess)
    {
        return false;
    }
    for (const String &metaFlag : flags)
    {
        if (TYPE_META_FLAGS.contains(metaFlag))
        {
            metaFlags.emplace_back(TCHAR("EClassMetaFlags::CLASSMETA_") + metaFlag);
        }
        else
        {
            buildFlags.emplace_back(metaFlag);
        }
    }
    return true;
}

bool ParserHelper::parseFieldMeta(
    std::vector<String> &metaFlags, std::vector<ParsedMetaData> &metaData, std::vector<String> &buildFlags, const String &annotatedStr
)
{
    static const std::unordered_set<String> TYPE_META_FLAGS{
        FOR_EACH_FIELD_META_FLAGS_UNIQUE_FIRST_LAST(META_FLAG_ENTRY_FIRST, META_FLAG_ENTRY, META_FLAG_ENTRY)
    };

    std::vector<String> flags;
    bool bSuccess = parseMeta(flags, metaData, annotatedStr);
    if (!bSuccess)
    {
        return false;
    }
    for (const String &metaFlag : flags)
    {
        if (TYPE_META_FLAGS.contains(metaFlag))
        {
            metaFlags.emplace_back(TCHAR("EFieldMetaFlags::FIELDMETA_") + metaFlag);
        }
        else
        {
            buildFlags.emplace_back(metaFlag);
        }
    }
    return true;
}

bool ParserHelper::parseFunctionMeta(
    std::vector<String> &metaFlags, std::vector<ParsedMetaData> &metaData, std::vector<String> &buildFlags, const String &annotatedStr
)
{
    static const std::unordered_set<String> TYPE_META_FLAGS{
        FOR_EACH_FUNC_META_FLAGS_UNIQUE_FIRST_LAST(META_FLAG_ENTRY_FIRST, META_FLAG_ENTRY, META_FLAG_ENTRY)
    };

    std::vector<String> flags;
    bool bSuccess = parseMeta(flags, metaData, annotatedStr);
    if (!bSuccess)
    {
        return false;
    }
    for (const String &metaFlag : flags)
    {
        if (TYPE_META_FLAGS.contains(metaFlag))
        {
            metaFlags.emplace_back(TCHAR("EFunctionMetaFlags::FUNCMETA_") + metaFlag);
        }
        else
        {
            buildFlags.emplace_back(metaFlag);
        }
    }
    return true;
}

bool ParserHelper::parseEnumMeta(
    std::vector<String> &metaFlags, std::vector<ParsedMetaData> &metaData, std::vector<String> &buildFlags, const String &annotatedStr
)
{
    static const std::unordered_set<String> TYPE_META_FLAGS{
        FOR_EACH_ENUM_META_FLAGS_UNIQUE_FIRST_LAST(META_FLAG_ENTRY_FIRST, META_FLAG_ENTRY, META_FLAG_ENTRY)
    };

    std::vector<String> flags;
    bool bSuccess = parseMeta(flags, metaData, annotatedStr);
    if (!bSuccess)
    {
        return false;
    }
    for (const String &metaFlag : flags)
    {
        if (TYPE_META_FLAGS.contains(metaFlag))
        {
            metaFlags.emplace_back(TCHAR("EEnumMetaFlags::ENUMMETA_") + metaFlag);
        }
        else
        {
            buildFlags.emplace_back(metaFlag);
        }
    }
    return true;
}
#undef META_FLAG_ENTRY_FIRST
#undef META_FLAG_ENTRY
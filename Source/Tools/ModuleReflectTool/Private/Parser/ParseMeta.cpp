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

// TODO(Jeslas) : Change ; to something better
std::vector<String> ParserHelper::parseMeta(std::vector<String> &metaData, const String &annotatedStr)
{
    std::vector<String> metaFlags;
    // We do ; termination for meta specifiers atleast for now just to make sure parsing is easier and
    // simple
    for (const StringView &metaView : String::split(annotatedStr, TCHAR(";")))
    {
        String meta{ metaView };
        if (PropertyHelper::isValidFunctionCall(meta.trim()))
        {
            metaData.emplace_back(std::move(meta));
        }
        else
        {
            metaFlags.emplace_back(std::move(meta));
        }
    }
    return metaFlags;
}

void ParserHelper::parseClassMeta(
    std::vector<String> &metaFlags, std::vector<String> &metaData, std::vector<String> &buildFlags, const String &annotatedStr
)
{
    static const std::unordered_set<String> TYPE_META_FLAGS{
        FOR_EACH_CLASS_META_FLAGS_UNIQUE_FIRST_LAST(META_FLAG_ENTRY_FIRST, META_FLAG_ENTRY, META_FLAG_ENTRY)
    };

    std::vector<String> flags = parseMeta(metaData, annotatedStr);
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
}

void ParserHelper::parseFieldMeta(
    std::vector<String> &metaFlags, std::vector<String> &metaData, std::vector<String> &buildFlags, const String &annotatedStr
)
{
    static const std::unordered_set<String> TYPE_META_FLAGS{
        FOR_EACH_FIELD_META_FLAGS_UNIQUE_FIRST_LAST(META_FLAG_ENTRY_FIRST, META_FLAG_ENTRY, META_FLAG_ENTRY)
    };

    std::vector<String> flags = parseMeta(metaData, annotatedStr);
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
}

void ParserHelper::parseFunctionMeta(
    std::vector<String> &metaFlags, std::vector<String> &metaData, std::vector<String> &buildFlags, const String &annotatedStr
)
{
    static const std::unordered_set<String> TYPE_META_FLAGS{
        FOR_EACH_FUNC_META_FLAGS_UNIQUE_FIRST_LAST(META_FLAG_ENTRY_FIRST, META_FLAG_ENTRY, META_FLAG_ENTRY)
    };

    std::vector<String> flags = parseMeta(metaData, annotatedStr);
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
}

void ParserHelper::parseEnumMeta(
    std::vector<String> &metaFlags, std::vector<String> &metaData, std::vector<String> &buildFlags, const String &annotatedStr
)
{
    static const std::unordered_set<String> TYPE_META_FLAGS{
        FOR_EACH_ENUM_META_FLAGS_UNIQUE_FIRST_LAST(META_FLAG_ENTRY_FIRST, META_FLAG_ENTRY, META_FLAG_ENTRY)
    };

    std::vector<String> flags = parseMeta(metaData, annotatedStr);
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
}
#undef META_FLAG_ENTRY_FIRST
#undef META_FLAG_ENTRY
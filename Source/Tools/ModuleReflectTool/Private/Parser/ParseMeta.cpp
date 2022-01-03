#include "Parser/ParserHelper.h"
#include "Property/PropertyHelper.h"
#include "Property/PropertyMetaFlags.inl"

#include <unordered_set>

#define META_FLAG_ENTRY_FIRST(Flag) #Flag
#define META_FLAG_ENTRY(Flag) , #Flag

// #TODO(Jeslas) : Change ; to something better
std::vector<String> ParserHelper::parseMeta(std::vector<String>& metaData, const String& annotatedStr)
{
    std::vector<String> metaFlags;
    // We do ; termination for meta specifiers atleast for now just to make sure parsing is easier and simple
    for (String& meta : String::split(annotatedStr, ";"))
    {
        if (PropertyHelper::isValidFunctionCall(meta.trim()))
        {
            metaData.emplace_back(meta);
        }
        else
        {
            metaFlags.emplace_back(meta);
        }
    }
    return metaFlags;
}

void ParserHelper::parseClassMeta(std::vector<String>& metaFlags, std::vector<String>& metaData, std::vector<String>& buildFlags, const String& annotatedStr)
{
    static const std::unordered_set<String> TYPE_META_FLAGS
    {
        FOR_EACH_CLASS_META_FLAGS_UNIQUE_FIRST_LAST(META_FLAG_ENTRY_FIRST, META_FLAG_ENTRY, META_FLAG_ENTRY)
    };

    std::vector<String> flags = parseMeta(metaData, annotatedStr);
    for (const String& metaFlag : flags)
    {
        if (TYPE_META_FLAGS.contains(metaFlag))
        {
            metaFlags.emplace_back("EClassMetaFlags::CLASSMETA_" + metaFlag);
        }
        else
        {
            buildFlags.emplace_back(metaFlag);
        }
    }
}

void ParserHelper::parseFieldMeta(std::vector<String>& metaFlags, std::vector<String>& metaData, std::vector<String>& buildFlags, const String& annotatedStr)
{
    static const std::unordered_set<String> TYPE_META_FLAGS
    {
        FOR_EACH_FIELD_META_FLAGS_UNIQUE_FIRST_LAST(META_FLAG_ENTRY_FIRST, META_FLAG_ENTRY, META_FLAG_ENTRY)
    };

    std::vector<String> flags = parseMeta(metaData, annotatedStr);
    for (const String& metaFlag : flags)
    {
        if (TYPE_META_FLAGS.contains(metaFlag))
        {
            metaFlags.emplace_back("EFieldMetaFlags::FIELDMETA_" + metaFlag);
        }
        else
        {
            buildFlags.emplace_back(metaFlag);
        }
    }
}

void ParserHelper::parseFunctionMeta(std::vector<String>& metaFlags, std::vector<String>& metaData, std::vector<String>& buildFlags, const String& annotatedStr)
{
    static const std::unordered_set<String> TYPE_META_FLAGS
    {
        FOR_EACH_FUNC_META_FLAGS_UNIQUE_FIRST_LAST(META_FLAG_ENTRY_FIRST, META_FLAG_ENTRY, META_FLAG_ENTRY)
    };

    std::vector<String> flags = parseMeta(metaData, annotatedStr);
    for (const String& metaFlag : flags)
    {
        if (TYPE_META_FLAGS.contains(metaFlag))
        {
            metaFlags.emplace_back("EFunctionMetaFlags::FUNCMETA_" + metaFlag);
        }
        else
        {
            buildFlags.emplace_back(metaFlag);
        }
    }
}

void ParserHelper::parseEnumMeta(std::vector<String>& metaFlags, std::vector<String>& metaData, std::vector<String>& buildFlags, const String& annotatedStr)
{
    static const std::unordered_set<String> TYPE_META_FLAGS
    {
        FOR_EACH_ENUM_META_FLAGS_UNIQUE_FIRST_LAST(META_FLAG_ENTRY_FIRST, META_FLAG_ENTRY, META_FLAG_ENTRY)
    };

    std::vector<String> flags = parseMeta(metaData, annotatedStr);
    for (const String& metaFlag : flags)
    {
        if (TYPE_META_FLAGS.contains(metaFlag))
        {
            metaFlags.emplace_back("EEnumMetaFlags::ENUMMETA_" + metaFlag);
        }
        else
        {
            buildFlags.emplace_back(metaFlag);
        }
    }
}
#undef META_FLAG_ENTRY_FIRST
#undef META_FLAG_ENTRY
/*!
 * \file ModuleReflectTypes.h
 *
 * \author Jeslas
 * \date April 2023
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "String/String.h"
#include "Types/Containers/ArrayView.h"
#include "String/MustacheFormatString.h"

#include <unordered_set>
#include <clang-c/Index.h>

class SourceGenerator;

struct ReflectedTypeItem
{
    // Canonical name
    String typeName;
    String includePath;
    String moduleName;

    bool operator== (const ReflectedTypeItem &rhs) const { return typeName == rhs.typeName; }
    bool operator< (const ReflectedTypeItem &rhs) const { return typeName < rhs.typeName; }

    static void fromString(std::vector<ReflectedTypeItem> &outReflectedTypes, StringView str);
    static String toString(ArrayView<ReflectedTypeItem> reflectedTypes);
};

template <>
struct std::hash<ReflectedTypeItem>
{
    NODISCARD SizeT operator() (const ReflectedTypeItem &keyval) const noexcept
    {
        auto stringHash = hash<String>();
        return stringHash(keyval.typeName);
    }
};

struct SourceInformation
{
    String filePath;
    String headerIncl;
    String generatedHeaderPath;
    String generatedTUPath;
    // TU will be parsed and be valid only if this source's reflection data is outdated.
    // Failing to parse will lead to termination. So this must be valid if this source is parsed
    CXTranslationUnit tu = nullptr;
    CXIndex index = nullptr;

    // File size used for sorting
    uint64 fileSize = 0;
};

struct alignas(CACHE_BREAK_SIZE) SourceGeneratorContext
{
    std::vector<MustacheContext> headerReflectTypes;
    std::vector<MustacheContext> allRegisteredypes;
    std::vector<MustacheContext> additionalIncludes;
    std::vector<MustacheContext> qualifiedTypes;
    std::vector<MustacheContext> pairTypes;
    std::vector<MustacheContext> containerTypes;
    std::vector<MustacheContext> mapTypes;
    std::vector<MustacheContext> enumTypes;
    std::vector<MustacheContext> classTypes;
    /**
     * List of symbols that are added in this generated TU
     * This will contain only types that are either qualified types or template types
     */
    std::unordered_set<String> addedSymbols;
    // Symbols that are reflected from other modules but are used in this source
    std::unordered_set<ReflectedTypeItem> externReflectSymbols;
    // If any error it will be set false
    bool bGenerated = true;

    // Read only data
    const SourceGenerator *generator = nullptr;
};

/*!
 * \file MustacheFormatString.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once
#include "ProgramCoreExports.h"
#include "String/StringFormat.h"
#include "String/StringRegex.h"

struct MustacheContext;
class MustacheStringFormatter;

//
// COPYRIGHT
// Mustache is Copyright (C) 2009 Chris Wanstrath
//
// Original CTemplate by Google
//
// SEE ALSO
// mustache(1), http://mustache.github.io/
//

using MustacheSectionFormatter = SingleCastDelegate<
    String, const MustacheStringFormatter &, const MustacheContext &, const std::unordered_map<String, MustacheStringFormatter> &>;
struct MustacheContext
{
    FormatArgsMap args;
    std::unordered_map<String, std::vector<MustacheContext>> sectionContexts;
    std::unordered_map<String, MustacheSectionFormatter> sectionFormatters;
};

// This is just a sub set of entire mustache tags
// There is no escaping of HTML tags happens, Everything is not escaped
// No support to replace delimiters
// only supports {{ }} for delimiters
// Supports sections branching and loops, Also supports section function formatter(You can use another
// function to invoke formatter with provided context or create new context for render) Supports not
// branch Supports partials Supports comments
class PROGRAMCORE_EXPORT MustacheStringFormatter
{
private:
    struct Section
    {
        // Match which is opening section tag
        uint32 sectionStartIdx;
        // Match which is closing section tag
        uint32 sectionEndIdx;
        // All childCount index after this section's idx will be this one's child
        uint32 childCount;
    };

private:
    String fmtStr;
    std::vector<StringMatch> allMatches;
    std::vector<Section> sections;
    void parseFmtStr();

    FORCE_INLINE bool isASection(const String &tagName) const { return tagName.startsWith(TCHAR('#')) || tagName.startsWith(TCHAR('^')); }
    FORCE_INLINE bool isANotSection(const String &tagName) const { return tagName.startsWith(TCHAR('^')); }
    FORCE_INLINE bool isSectionClose(const String &tagName) const { return tagName.startsWith(TCHAR('/')); }
    FORCE_INLINE bool isAComment(const String &tagName) const { return tagName.startsWith(TCHAR('!')); }
    FORCE_INLINE bool isAPartial(const String &tagName) const { return tagName.startsWith(TCHAR('>')); }
    FORCE_INLINE void removeMustachePrefix(String &tagName) const;

    void renderSection(
        OStringStream &outStr, uint32 sectionIdx, const MustacheContext &context,
        const std::unordered_map<String, MustacheStringFormatter> &partials
    ) const;
    FORCE_INLINE void renderSectionInner(
        OStringStream &outStr, const Section &section, const MustacheContext &context,
        const std::unordered_map<String, MustacheStringFormatter> &partials
    ) const;
    // Returns next match idx to render
    uint32 renderTag(
        OStringStream &outStr, uint32 matchIdx, const MustacheContext &context,
        const std::unordered_map<String, MustacheStringFormatter> &partials
    ) const;

public:
    MustacheStringFormatter() = default;
    MustacheStringFormatter(const String &fmt);
    FORCE_INLINE MustacheStringFormatter(const MustacheStringFormatter &other)
        : fmtStr(other.fmtStr)
    {
        parseFmtStr();
    }
    FORCE_INLINE MustacheStringFormatter(MustacheStringFormatter &&other)
        : fmtStr(std::move(other.fmtStr))
    {
        parseFmtStr();
    }
    FORCE_INLINE MustacheStringFormatter &operator=(const MustacheStringFormatter &other)
    {
        fmtStr = other.fmtStr;
        parseFmtStr();
        return *this;
    }
    FORCE_INLINE MustacheStringFormatter &operator=(MustacheStringFormatter &&other)
    {
        fmtStr = std::move(other.fmtStr);
        parseFmtStr();
        return *this;
    }

    // Just replaces all the tag with value(Assumes all tags in format as basic Variables)
    // Ignores comment tags
    // Efficient with very less string copies and parallel executable(Not yet)
    // Function getter string must be providing constant value during format as they are cached before
    // final copy
    String formatBasic(const FormatArgsMap &formatArgs) const;

    // Warning use partials with caution to avoid infinite recursions, When using partials try to wrap
    // around a branch with lambda condition check
    String render(const MustacheContext &context, const std::unordered_map<String, MustacheStringFormatter> &partials) const;
};

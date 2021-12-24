#pragma once
#include "String/StringFormat.h"
#include "ProgramCoreExports.h"

#include <regex>

struct MustacheContext;
class MustacheStringFormatter;

//
//COPYRIGHT
//Mustache is Copyright (C) 2009 Chris Wanstrath
//
//Original CTemplate by Google
//
//SEE ALSO
//mustache(1), http://mustache.github.io/
//

using MustacheSectionFormatter = SingleCastDelegate<String, const MustacheStringFormatter&, const MustacheContext&>;
struct MustacheContext
{
    const FormatArgsMap* args;
    const std::unordered_map<String, std::vector<FormatArgsMap>>* sectionArgs;
    const std::unordered_map<String, MustacheSectionFormatter>* sectionFormatters;
    const std::unordered_map<String, MustacheStringFormatter>* partials;
};

// This is just a sub set of entire mustache tags
// There is no escaping of HTML tags happens, Everything is not escaped
// No support to replace delimiters
// only supports {{ }} for delimiters
// Supports sections branching and loops, Also supports section function formatter(You can use another function to invoke formatter with provided context or create new context for render)
// Supports not branch
// Supports partials
// Supports comments
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
    std::vector<std::smatch> allMatches;
    std::vector<Section> sections;

    FORCE_INLINE bool isASection(const String& tagName) const { return tagName.startsWith("#") || tagName.startsWith("^"); }
    FORCE_INLINE bool isANotSection(const String& tagName) const { return tagName.startsWith("^"); }
    FORCE_INLINE bool isSectionClose(const String& tagName) const { return tagName.startsWith("/"); }
    FORCE_INLINE bool isAComment(const String& tagName) const { return tagName.startsWith("!"); }
    FORCE_INLINE bool isAPartial(const String& tagName) const { return tagName.startsWith(">"); }
    FORCE_INLINE void removeMustachePrefix(String& tagName) const;

    void renderSection(std::ostringstream& outStr, uint32 sectionIdx, const FormatArgsMap& args, const std::unordered_map<String, std::vector<FormatArgsMap>>& sectionArgs
        , const std::unordered_map<String, MustacheSectionFormatter>& sectionFormatters
        , const std::unordered_map<String, MustacheStringFormatter>& partials) const;
    // Returns next match idx to render 
    uint32 renderTag(std::ostringstream& outStr, uint32 matchIdx, const FormatArgsMap& args, const std::unordered_map<String, std::vector<FormatArgsMap>>& sectionArgs
        , const std::unordered_map<String, MustacheSectionFormatter>& sectionFormatters
        , const std::unordered_map<String, MustacheStringFormatter>& partials) const;
public:
    MustacheStringFormatter(const String& fmt);

    // Just replaces all the tag with value(Assumes all tags in format as basic Variables)
    // Ignores comment tags
    // Efficient with very less string copies and parallel executable(Not yet)
    // Function getter string must be providing constant value during format as they are cached before final copy
    String formatBasic(const FormatArgsMap& formatArgs) const;

    // Warning use partials with caution to avoid infinite recursions, When using partials try to wrap around a branch with lambda condition check
    FORCE_INLINE String render(const FormatArgsMap& args) const
    {
        return render(args, {}, {}, {});
    }
    FORCE_INLINE String render(const FormatArgsMap& args, const std::unordered_map<String, std::vector<FormatArgsMap>>& sectionArgs) const
    {
        return render(args, sectionArgs, {}, {});
    }
    String render(const FormatArgsMap& args, const std::unordered_map<String, std::vector<FormatArgsMap>>& sectionArgs
        , const std::unordered_map<String, MustacheSectionFormatter>& sectionFormatters
        , const std::unordered_map<String, MustacheStringFormatter>& partials) const;
};
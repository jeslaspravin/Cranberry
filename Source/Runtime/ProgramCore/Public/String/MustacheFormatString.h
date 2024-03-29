/*!
 * \file MustacheFormatString.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once
#include "ProgramCoreExports.h"
#include "Types/Delegates/Delegate.h"
#include "String/StringFormat.h"
#include "String/StringRegex.h"

#include <unordered_map>

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

struct PROGRAMCORE_EXPORT MustacheFormatArg
{
    using ArgGetter = SingleCastDelegate<String>;
    union ArgValue
    {
        String strVal;
        ArgGetter argGetter;
        CoreTypesUnion fundamentalVals;

        ArgValue(CoreTypesUnion inFundamentalVals)
            : fundamentalVals(inFundamentalVals)
        {}
        template <typename StrType>
        requires StringType<StrType>
        ArgValue(StrType &&inStrVal)
            : strVal(std::forward<StrType>(inStrVal))
        {}
        ArgValue(ArgGetter &&getter)
            : argGetter(std::forward<ArgGetter>(getter))
        {}
        ArgValue()
            : strVal()
        {}
        ~ArgValue() {}
    };
    ArgValue value;
    enum
    {
        NoType,
        Bool,
        UInt8,
        UInt16,
        UInt32,
        UInt64,
        Int8,
        Int16,
        Int32,
        Int64,
        Float,
        Double,
        Getter,
        AsString // Whatever other type that will be stored as string after toString conversion
    } type;

    MustacheFormatArg() noexcept
        : value()
        , type(NoType)
    {}
    MustacheFormatArg(bool argValue) noexcept
        : type(Bool)
    {
        value.fundamentalVals.boolVal = argValue;
    }
    MustacheFormatArg(uint8 argValue) noexcept
        : type(UInt8)
    {
        value.fundamentalVals.uint8Val = argValue;
    }
    MustacheFormatArg(uint16 argValue) noexcept
        : type(UInt16)
    {
        value.fundamentalVals.uint16Val = argValue;
    }
    MustacheFormatArg(uint32 argValue) noexcept
        : type(UInt32)
    {
        value.fundamentalVals.uint32Val = argValue;
    }
    MustacheFormatArg(uint64 argValue) noexcept
        : type(UInt64)
    {
        value.fundamentalVals.uint64Val = argValue;
    }
    MustacheFormatArg(int8 argValue) noexcept
        : type(Int8)
    {
        value.fundamentalVals.int8Val = argValue;
    }
    MustacheFormatArg(int16 argValue) noexcept
        : type(Int16)
    {
        value.fundamentalVals.int16Val = argValue;
    }
    MustacheFormatArg(int32 argValue) noexcept
        : type(Int32)
    {
        value.fundamentalVals.int32Val = argValue;
    }
    MustacheFormatArg(int64 argValue) noexcept
        : type(Int64)
    {
        value.fundamentalVals.int64Val = argValue;
    }
    MustacheFormatArg(float argValue) noexcept
        : type(Float)
    {
        value.fundamentalVals.floatVal = argValue;
    }
    MustacheFormatArg(double argValue) noexcept
        : type(Double)
    {
        value.fundamentalVals.doubleVal = argValue;
    }
    MustacheFormatArg(ArgGetter &&argValue) noexcept
        : value(std::forward<ArgGetter>(argValue))
        , type(Getter)
    {}
    template <typename InType>
    MustacheFormatArg(InType &&argValue) noexcept;

    MustacheFormatArg(const MustacheFormatArg &arg) noexcept;
    MustacheFormatArg(MustacheFormatArg &&arg) noexcept;
    MustacheFormatArg &operator= (const MustacheFormatArg &arg) noexcept;
    MustacheFormatArg &operator= (MustacheFormatArg &&arg) noexcept;

    String toString() const noexcept;
    operator bool () const noexcept;
};
using FormatArgsMap = std::unordered_map<String, MustacheFormatArg>;

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
// Supports sections branching and loops
// Supports section formatter delegates(You can use another function to invoke formatter with provided context or create new context for render)
// Supports not branch
// Supports partials
// Supports comments
//
// Additionally when ever there is a sectionContexts(Loop), supports indexing those loops
// Loop indices are populated with argument __$idx0__, __$idx1__ ... __idx<N>__ for N level nested loop
//

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
    void parseFmtStr() noexcept;

    constexpr static const TChar *INDEX_FMT = TCHAR("__idx%u__");

    FORCE_INLINE bool isASection(const String &tagName) const noexcept
    {
        return tagName.startsWith(TCHAR('#')) || tagName.startsWith(TCHAR('^'));
    }
    FORCE_INLINE bool isANotSection(const String &tagName) const noexcept { return tagName.startsWith(TCHAR('^')); }
    FORCE_INLINE bool isSectionClose(const String &tagName) const noexcept { return tagName.startsWith(TCHAR('/')); }
    FORCE_INLINE bool isAComment(const String &tagName) const noexcept { return tagName.startsWith(TCHAR('!')); }
    FORCE_INLINE bool isAPartial(const String &tagName) const noexcept { return tagName.startsWith(TCHAR('>')); }
    FORCE_INLINE void removeMustachePrefix(String &tagName) const noexcept;

    void renderSection(
        OStringStream &outStr, uint32 sectionIdx, const MustacheContext &context,
        const std::unordered_map<String, MustacheStringFormatter> &partials
    ) const noexcept;
    FORCE_INLINE void renderSectionInner(
        OStringStream &outStr, const Section &section, const MustacheContext &context,
        const std::unordered_map<String, MustacheStringFormatter> &partials
    ) const noexcept;
    // Returns next match idx to render
    uint32 renderTag(
        OStringStream &outStr, uint32 matchIdx, const MustacheContext &context,
        const std::unordered_map<String, MustacheStringFormatter> &partials
    ) const noexcept;

public:
    MustacheStringFormatter() = default;
    MustacheStringFormatter(const String &fmt) noexcept;
    FORCE_INLINE MustacheStringFormatter(const MustacheStringFormatter &other) noexcept
        : fmtStr(other.fmtStr)
    {
        parseFmtStr();
    }
    FORCE_INLINE MustacheStringFormatter(MustacheStringFormatter &&other) noexcept
        : fmtStr(std::move(other.fmtStr))
    {
        parseFmtStr();
    }
    FORCE_INLINE MustacheStringFormatter &operator= (const MustacheStringFormatter &other) noexcept
    {
        fmtStr = other.fmtStr;
        parseFmtStr();
        return *this;
    }
    FORCE_INLINE MustacheStringFormatter &operator= (MustacheStringFormatter &&other) noexcept
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
    String formatBasic(const FormatArgsMap &formatArgs) const noexcept;

    // Warning use partials with caution to avoid infinite recursions, When using partials try to wrap
    // around a branch with lambda condition check
    String render(const MustacheContext &context, const std::unordered_map<String, MustacheStringFormatter> &partials) const noexcept;

    static String formatMustache(const String &fmt, const FormatArgsMap &formatArgs)
    {
        MustacheStringFormatter formatter(fmt);
        return formatter.formatBasic(formatArgs);
    }
};

template <typename InType>
MustacheFormatArg::MustacheFormatArg(InType &&argValue) noexcept
    : value(StringFormat::toString<InType>(std::forward<InType>(argValue)))
    , type(MustacheFormatArg::AsString)
{}
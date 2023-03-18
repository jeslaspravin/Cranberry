/*!
 * \file PropertyMetaFlags.inl
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

// clang-format off

/**
 * Currently meta flags can be just normal unsigned integer. Starting from 0 to 63. When using to check flag bits always use INDEX_TO_FLAG_MASK(EnumVal) to make bit flag
 */

// Class/Struct meta
#define FOR_EACH_CLASS_META_FLAGS_UNIQUE_FIRST_LAST(FirstMacroName, MacroName, LastMacroName)

#define FOR_EACH_CLASS_META_FLAGS(MacroName) FOR_EACH_CLASS_META_FLAGS_UNIQUE_FIRST_LAST(MacroName, MacroName, MacroName)

// Enum meta
#define FOR_EACH_ENUM_META_FLAGS_UNIQUE_FIRST_LAST(FirstMacroName, MacroName, LastMacroName)

#define FOR_EACH_ENUM_META_FLAGS(MacroName) FOR_EACH_ENUM_META_FLAGS_UNIQUE_FIRST_LAST(MacroName, MacroName, MacroName)

// Field meta
#define FOR_EACH_FIELD_META_FLAGS_UNIQUE_FIRST_LAST(FirstMacroName, MacroName, LastMacroName)   \
    FirstMacroName(Transient)

#define FOR_EACH_FIELD_META_FLAGS(MacroName) FOR_EACH_FIELD_META_FLAGS_UNIQUE_FIRST_LAST(MacroName, MacroName, MacroName)

// Function meta
#define FOR_EACH_FUNC_META_FLAGS_UNIQUE_FIRST_LAST(FirstMacroName, MacroName, LastMacroName)

#define FOR_EACH_FUNC_META_FLAGS(MacroName) FOR_EACH_FUNC_META_FLAGS_UNIQUE_FIRST_LAST(MacroName, MacroName, MacroName)

// clang-format on
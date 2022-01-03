#pragma once

// Class/Struct meta
#define FOR_EACH_CLASS_META_FLAGS_UNIQUE_FIRST_LAST(FirstMacroName, MacroName, LastMacroName) \

#define FOR_EACH_CLASS_META_FLAGS(MacroName) FOR_EACH_CLASS_META_FLAGS_UNIQUE_FIRST_LAST(MacroName, MacroName, MacroName)

// Enum meta
#define FOR_EACH_ENUM_META_FLAGS_UNIQUE_FIRST_LAST(FirstMacroName, MacroName, LastMacroName) \

#define FOR_EACH_ENUM_META_FLAGS(MacroName) FOR_EACH_ENUM_META_FLAGS_UNIQUE_FIRST_LAST(MacroName, MacroName, MacroName)

// Field meta
#define FOR_EACH_FIELD_META_FLAGS_UNIQUE_FIRST_LAST(FirstMacroName, MacroName, LastMacroName) \

#define FOR_EACH_FIELD_META_FLAGS(MacroName) FOR_EACH_FIELD_META_FLAGS_UNIQUE_FIRST_LAST(MacroName, MacroName, MacroName)

// Function meta
#define FOR_EACH_FUNC_META_FLAGS_UNIQUE_FIRST_LAST(FirstMacroName, MacroName, LastMacroName) \

#define FOR_EACH_FUNC_META_FLAGS(MacroName) FOR_EACH_FUNC_META_FLAGS_UNIQUE_FIRST_LAST(MacroName, MacroName, MacroName)
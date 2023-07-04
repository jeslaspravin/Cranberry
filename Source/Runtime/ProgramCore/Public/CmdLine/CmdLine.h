/*!
 * \file CmdLine.h
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
#include "String/String.h"
#include "Types/CoreTypes.h"

struct DEPRECATED("Use REGISTER_CMDARG macros instead") CmdLineArgument
{
    // Short arg is for flags alone
    PROGRAMCORE_EXPORT CmdLineArgument(StringView description, StringView cmdArg, StringView shortArg = TCHAR(""));
};

// Use if want to have static registration of command line arguments
#define REGISTER_CMDARG_S(Desc, CmdArgName, ShortArgName)                                                                                      \
    DISABLE_DEPRECATION                                                                                                                        \
    static const CmdLineArgument COMBINE(zzzCmdLineArg_, __COUNTER__){ TCHAR(Desc), CmdArgName, ShortArgName };                                \
    ENABLE_DEPRECATION
#define REGISTER_CMDARG(Desc, CmdArgName)                                                                                                      \
    DISABLE_DEPRECATION                                                                                                                        \
    static const CmdLineArgument COMBINE(zzzCmdLineArg_, __COUNTER__){ TCHAR(Desc), CmdArgName };                                              \
    ENABLE_DEPRECATION

/*
 * Args starting with `single -` will be short hand flags and can be stringed together and they cannot
 * accept values. Args starting with `--` Can have values space delimited strings Args starting with `@`
 * refers to file which contains all arguments(It can be absolute path or relative to application
 * directory) All args are case sensitive
 */
class PROGRAMCORE_EXPORT ProgramCmdLine
{
private:
    // Args parsed from file/Multibyte chars are placed here
    String argsCache;

protected:
    ProgramCmdLine() = default;

    String programDescription;
    std::vector<StringView> cmdLineElements;

private:
    bool parseFromFile(const String &filePath);
    bool parseViews(const std::vector<StringView> &strViews) noexcept;

public:
    static ProgramCmdLine &get();
    // Takes the pointer as it is and hold it for all parsing and data retrieval
    bool parse(AChar **cmdArgs, uint32 count);
    // Takes the reference as it is and uses the TChar* to the cmdLine references
    // NOTE: The String passed in must live throughout the life time of ProgramCmdLine
    bool parse(const String &cmdLine);
    void setProgramDescription(const String &description) { programDescription = description; }

    // Prints help if the command line has asked for it and returns true if this cmd is invoked to print
    // help. Must be called after parsing the cmd line
    bool printHelp() const;
    void printCommandLine() const;

    // Must be queried with `-` or `--`
    bool hasArg(const String &argName) const;
    bool getArg(String &outValue, const String &argName) const;
    bool getArg(std::vector<String> &outValues, const String &argName) const;

    // Gets command line value at index
    String atIdx(uint32 idx) const;
    uint32 cmdLineCount() const;
};
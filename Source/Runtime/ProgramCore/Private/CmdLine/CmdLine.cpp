/*!
 * \file CmdLine.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "CmdLine/CmdLine.h"
#include "Logger/Logger.h"
#include "Types/Platform/LFS/File/FileHelper.h"
#include "Types/Platform/LFS/PathFunctions.h"
#include "Types/Platform/LFS/Paths.h"
#include "Types/Platform/PlatformAssertionErrors.h"

#include <unordered_map>

class ProgramCmdLineInstance : public ProgramCmdLine
{
public:
    struct AllowedArg
    {
        String argName;
        String shortArgName;
        String argDescription;
    };
    struct ArgElementsRange
    {
        // Index of arg's key, -1 means not found
        int32 argIdx;
        // start index of arg's value/s
        int32 argValueIdx;
        // Count of values in valued arg and offset of char in flag arg
        int32 count;
    };
    std::vector<AllowedArg> allowedArgs;
    std::unordered_map<String, ArgElementsRange> cmdArgs;

public:
    void addAllowedArg(const String &cmdArg, const String &shortArg, const String &description)
    {
        allowedArgs.emplace_back(AllowedArg{ cmdArg, shortArg, description });
    }

    void addCmdArg(const String &argName, const ArgElementsRange &range)
    {
        for (AllowedArg &allowedArg : allowedArgs)
        {
            // If matches long arg then only if short arg matches we add short arg and value
            if (allowedArg.argName == argName)
            {
                cmdArgs.insert({ argName, range });
                if (!allowedArg.shortArgName.empty())
                {
                    cmdArgs.insert({ allowedArg.shortArgName, range });
                }
                return;
            }
            else if (allowedArg.shortArgName == argName)
            {
                cmdArgs.insert({ argName, range });
                cmdArgs.insert({ allowedArg.argName, range });
                return;
            }
        }

        // If no allowed args found then just add it directly
        cmdArgs.insert({ argName, range });
    }

    void parseArgElements();
};

CmdLineArgument::CmdLineArgument(String description, String cmdArg, String shortArg /*= ""*/)
{
    static_cast<ProgramCmdLineInstance *>(&ProgramCmdLine::get())->addAllowedArg(cmdArg, shortArg, description);
}

ProgramCmdLine &ProgramCmdLine::get()
{
    static ProgramCmdLineInstance singletonProgramCmdLine;
    return static_cast<ProgramCmdLine &>(singletonProgramCmdLine);
}

bool ProgramCmdLine::parseViews(const std::vector<StringView> &strViews)
{
    bool bSuccess = true;

    // Parse from each view
    cmdLineElements.clear();
    cmdLineElements.reserve(strViews.size());
    for (const StringView &cmdArgView : strViews)
    {
        // Do not need to trim spaces as they must already trimmed before reaching this point
        // Parse from file
        if (cmdArgView.starts_with(TCHAR('@')))
        {
            // Skip @ char
            String filePath(cmdArgView.substr(1, cmdArgView.length() - 1));
            // If it is quoted path then remove first and last
            if (filePath.startsWith(TCHAR("\"")) || filePath.startsWith(TCHAR("'")))
            {
                filePath.replace(0, 1, TCHAR("")).replace(filePath.length() - 1, 1, TCHAR(""));
            }
            bSuccess = parseFromFile(filePath);
            break;
        }
        else if (!cmdArgView.starts_with(TCHAR('\0')))
        {
            cmdLineElements.emplace_back(cmdArgView);
        }
    }

    ProgramCmdLineInstance *thisInst = static_cast<ProgramCmdLineInstance *>(this);
    thisInst->parseArgElements();
    return bSuccess;
}

bool ProgramCmdLine::parseFromFile(const String &filePath)
{
    String appDir = Paths::applicationDirectory();
    String argFilePath = PathFunctions::toAbsolutePath(filePath, appDir);

    if (FileHelper::readString(argsCache, argFilePath))
    {
        std::vector<StringView> views = argsCache.splitLines();
        // If only one line there is a chance that line contains entire cmd line so do regular parsing
        if (views.size() == 1)
        {
            return parse(argsCache);
        }
        else
        {
            return parseViews(views);
        }
    }
    return false;
}

bool ProgramCmdLine::parse(AChar **cmdArgs, uint32 count)
{
    // Fill args cache and update data for string view
    argsCache.clear();
    std::vector<std::pair<uint64, uint64>> cmdArgIdxRange;
    cmdArgIdxRange.resize(count);
    for (uint32 i = 0; i < count; ++i)
    {
        cmdArgIdxRange[i].first = argsCache.length();
        argsCache.append(UTF8_TO_TCHAR(cmdArgs[i]));
        cmdArgIdxRange[i].second = argsCache.length();
        argsCache.append(TCHAR(" "));
    }

    std::vector<StringView> views(count);
    for (uint32 i = 0; i < count; ++i)
    {
        views.emplace(views.begin() + i, argsCache.cbegin() + cmdArgIdxRange[i].first, argsCache.cbegin() + cmdArgIdxRange[i].second);
    }
    return parseViews(views);
}

bool ProgramCmdLine::parse(const String &cmdLine)
{
    bool bSuccess = true;
    cmdLineElements.clear();

    auto IterateTillNextNonSpace = [](auto tokenItr, auto endTokenItr)
    {
        while (tokenItr != endTokenItr && std::isspace(*tokenItr))
        {
            tokenItr++;
        }
        return tokenItr;
    };
    auto IterateTillNextSpace = [](auto tokenItr, auto endTokenItr)
    {
        while (tokenItr != endTokenItr && !std::isspace(*tokenItr))
        {
            tokenItr++;
        }
        return tokenItr;
    };
    auto IterateTillEndQoute = [](auto tokenItr, auto quoteTokenItr, auto endTokenItr)
    {
        while (tokenItr != endTokenItr && (*tokenItr) != (*quoteTokenItr))
        {
            tokenItr++;
        }
        return tokenItr;
    };

    for (auto tokenItr = cmdLine.cbegin(); tokenItr != cmdLine.cend();)
    {
        tokenItr = IterateTillNextNonSpace(tokenItr, cmdLine.cend());
        auto endTokenItr = tokenItr;
        // If it is file reference then parse from file
        if ((*endTokenItr) == TCHAR('@'))
        {
            ++endTokenItr;
            tokenItr = endTokenItr;
            if ((*endTokenItr) == TCHAR('"') || (*endTokenItr) == TCHAR('\''))
            {
                tokenItr = endTokenItr + 1;
                endTokenItr = IterateTillEndQoute(tokenItr, endTokenItr, cmdLine.cend());
            }
            else
            {
                endTokenItr = IterateTillNextSpace(endTokenItr, cmdLine.cend());
            }

            bSuccess = parseFromFile(String(tokenItr, endTokenItr));
            break;
        }
        else if ((*endTokenItr) == TCHAR('"') || (*endTokenItr) == TCHAR('\'')) // If quoted value then end iterator is at the closing quote
        {
            tokenItr = endTokenItr + 1;
            endTokenItr = IterateTillEndQoute(tokenItr, endTokenItr, cmdLine.cend());
        }
        else // Find until next space delimiter
        {
            endTokenItr = IterateTillNextSpace(endTokenItr, cmdLine.cend());
        }

        if (endTokenItr != tokenItr)
        {
            cmdLineElements.emplace_back(StringView(tokenItr, endTokenItr));
        }
        // endItr can be either at the end or at a space char or at a quote char
        // In case of quote character we need to shift it by one for next iteration to work good
        tokenItr = (endTokenItr == cmdLine.cend() ? endTokenItr : endTokenItr + 1);
    }

    ProgramCmdLineInstance *thisInst = static_cast<ProgramCmdLineInstance *>(this);
    thisInst->parseArgElements();

    return bSuccess;
}

void ProgramCmdLineInstance::parseArgElements()
{
    cmdArgs.clear();

    for (auto itr = cmdLineElements.cbegin(); itr != cmdLineElements.cend();)
    {
        // If starts with - then this itr must be a arg start
        if (itr->starts_with(TCHAR('-')))
        {
            auto currentArgItr = itr;
            // Can have value check next itrs for value
            if (itr->starts_with(TCHAR("--")))
            {
                itr = itr + 1;
                // Every element that does not start with `-` after current arg will be value
                // for this
                ArgElementsRange argRange;
                argRange.argIdx = int32(currentArgItr - cmdLineElements.cbegin());
                argRange.argValueIdx = int32(itr - cmdLineElements.cbegin());
                argRange.count = 0;
                for (; itr != cmdLineElements.cend() && !itr->starts_with('-'); ++itr)
                {
                    ++argRange.count;
                }
                // If count is 0 it means this arg must be either flag or without any values
                if (argRange.count == 0)
                {
                    argRange.argValueIdx = -1;
                }

                addCmdArg({ *currentArgItr }, argRange);
            }
            else // This is flag/s
            {
                // +1 skips `-`
                auto flagsItr = currentArgItr->cbegin() + 1;
                for (; flagsItr != currentArgItr->cend(); ++flagsItr)
                {
                    String argName = TCHAR("-") + String(1, *flagsItr);
                    // Insert each flags separately prefixed with `-`
                    addCmdArg(
                        argName, ArgElementsRange{ .argIdx = int32(currentArgItr - cmdLineElements.cbegin()),
                                                   .argValueIdx = -1,
                                                   .count = int32(flagsItr - currentArgItr->cbegin()) }
                    );
                }
                ++itr;
            }
        }
        else
        {
            // Skip this element
            ++itr;
        }
    }
}

bool ProgramCmdLine::printHelp() const
{
    // Remove mute in this scope if log is muted
    SCOPED_MUTE_LOG_SEVERITIES(0);

    const ProgramCmdLineInstance *thisInst = static_cast<const ProgramCmdLineInstance *>(this);
    auto itrHelp = thisInst->cmdArgs.find(TCHAR("--help"));
    if (itrHelp == thisInst->cmdArgs.cend())
    {
        itrHelp = thisInst->cmdArgs.find(TCHAR("-h"));
    }
    if (itrHelp != thisInst->cmdArgs.cend())
    {
        String outHelp = programDescription + TCHAR("\n");
        for (const auto &allowedArg : thisInst->allowedArgs)
        {
            if (allowedArg.shortArgName.empty())
            {
                outHelp += StringFormat::printf(TCHAR("\n\"%s\"\n    - %s"), allowedArg.argName, allowedArg.argDescription);
            }
            else
            {
                outHelp += StringFormat::printf(
                    TCHAR("\n\"%s\", \"%s\"\n    - %s"), allowedArg.argName, allowedArg.shortArgName, allowedArg.argDescription
                );
            }
        }
        LOG("CmdLineHelp", "\n[HELP]\n%s\n", outHelp);
        return true;
    }
    return false;
}

void ProgramCmdLine::printCommandLine() const
{
    // Remove mute in this scope if log is muted
    SCOPED_MUTE_LOG_SEVERITIES(0);

    String appName = Paths::applicationName();

    String cmdLine;
    for (uint32 i = 1; i < cmdLineElements.size(); ++i)
    {
        cmdLine += TCHAR(" ") + String(cmdLineElements[i]);
    }
    LOG("CommandLine", "%s%s", appName, cmdLine);
}

bool ProgramCmdLine::hasArg(const String &argName) const
{
    const ProgramCmdLineInstance *thisInst = static_cast<const ProgramCmdLineInstance *>(this);
    auto argItr = thisInst->cmdArgs.find(argName);
    return thisInst->cmdArgs.cend() != argItr;
}

bool ProgramCmdLine::getArg(String &outValue, const String &argName) const
{
    const ProgramCmdLineInstance *thisInst = static_cast<const ProgramCmdLineInstance *>(this);
    auto argItr = thisInst->cmdArgs.find(argName);
    if (thisInst->cmdArgs.cend() != argItr && argItr->second.count > 0)
    {
        outValue = cmdLineElements[argItr->second.argValueIdx];
        return true;
    }
    return false;
}

bool ProgramCmdLine::getArg(std::vector<String> &outValues, const String &argName) const
{
    const ProgramCmdLineInstance *thisInst = static_cast<const ProgramCmdLineInstance *>(this);
    auto argItr = thisInst->cmdArgs.find(argName);
    if (thisInst->cmdArgs.cend() != argItr && argItr->second.count > 0)
    {
        outValues.insert(
            outValues.begin(), cmdLineElements.cbegin() + argItr->second.argValueIdx,
            cmdLineElements.cbegin() + argItr->second.argValueIdx + argItr->second.count
        );
        return true;
    }
    return false;
}

String ProgramCmdLine::atIdx(uint32 idx) const
{
    fatalAssertf(cmdLineElements.size() > idx, "Cmd line value idx %d out of range %lu", idx, cmdLineElements.size());
    return cmdLineElements[idx];
}

uint32 ProgramCmdLine::cmdLineCount() const { return uint32(cmdLineElements.size()); }

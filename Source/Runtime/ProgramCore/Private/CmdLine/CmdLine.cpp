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
#include "Types/Platform/LFS/PlatformLFS.h"

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
    void addAllowedArg(const String& cmdArg, const String& shortArg, const String& description)
    {
        allowedArgs.emplace_back(AllowedArg{ cmdArg, shortArg, description });
    }

    void addCmdArg(const String& argName, const ArgElementsRange& range)
    {
        for (AllowedArg& allowedArg : allowedArgs)
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
    static_cast<ProgramCmdLineInstance*>(ProgramCmdLine::get())->addAllowedArg(cmdArg, shortArg, description);
}

ProgramCmdLine* ProgramCmdLine::get()
{
    static ProgramCmdLineInstance instance;
    return &instance;
}

bool ProgramCmdLine::parseFromFile(const String& filePath)
{
    String appDir;
    appDir = FileSystemFunctions::applicationDirectory(appDir);
    String argFilePath = PathFunctions::toAbsolutePath(filePath, appDir);

    PlatformFile argFile(argFilePath);
    argFile.setFileFlags(EFileFlags::Read);
    argFile.setSharingMode(EFileSharing::ReadOnly);
    argFile.setCreationAction(EFileFlags::OpenExisting);
    if (argFile.exists())
    {
        argFile.openFile();        
        argFile.read(argsFromFile);
        argFile.closeFile();

        return parse(argsFromFile);
    }
    return false;
}

bool ProgramCmdLine::parse(AChar* cmdArgs[], uint32 count)
{
    bool bSuccess = true;
    cmdLineElements.clear();
    cmdLineElements.reserve(count);
    for (uint32 i = 0; i < count; ++i)
    {
        AChar* cmdArg = cmdArgs[i];
        // Do not need to trim spaces as they must already trimmed before reaching this point
        //while ((*cmdArg) != '\0' && std::isspace(*cmdArg))
        //{
        //    cmdArg++;
        //}

        // Parse from file
        if ((*cmdArg) == '@')
        {
            String filePath(cmdArg + 1);
            // If it is quoted path then remove first and last
            if (filePath.startsWith("\"") || filePath.startsWith("'"))
            {
                filePath.replace(0, 1, "").replace(filePath.length() - 1, 1, "");
            }
            bSuccess = parseFromFile(filePath);
            break;
        }
        else if ((*cmdArg) != '\0')
        {
            cmdLineElements.emplace_back(std::string_view(cmdArg));
        }
    }

    ProgramCmdLineInstance* thisInst = static_cast<ProgramCmdLineInstance*>(this);
    thisInst->parseArgElements();
    return bSuccess;
}

bool ProgramCmdLine::parse(const String& cmdLine)
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
        if ((*endTokenItr) == '@')
        {
            ++endTokenItr;
            tokenItr = endTokenItr;
            if ((*endTokenItr) == '"' || (*endTokenItr) == '\'')
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
        else if ((*endTokenItr) == '"' || (*endTokenItr) == '\'')// If quoted value then end iterator is at the closing quote
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
            cmdLineElements.emplace_back(std::string_view(tokenItr, endTokenItr));
        }
        // endItr can be either at the end or at a space char or at a quote char
        // In case of quote character we need to shift it by one for next iteration to work good
        tokenItr = (endTokenItr == cmdLine.cend()
            ? endTokenItr 
            : endTokenItr + 1
        );
    }

    ProgramCmdLineInstance* thisInst = static_cast<ProgramCmdLineInstance*>(this);
    thisInst->parseArgElements();

    return bSuccess;
}

void ProgramCmdLineInstance::parseArgElements()
{
    cmdArgs.clear();

    for (auto itr = cmdLineElements.cbegin(); itr != cmdLineElements.cend();)
    {
        // If starts with - then this itr must be a arg start
        if (itr->starts_with('-'))
        {
            auto currentArgItr = itr;
            // Can have value check next itrs for value
            if (itr->starts_with("--"))
            {
                itr = itr + 1;
                // Every element that does not start with `-` after current arg will be value for this 
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

                addCmdArg({*currentArgItr}, argRange);
            }
            else // This is flag/s
            {
                // +1 skips `-`
                auto flagsItr = currentArgItr->cbegin() + 1;
                for (; flagsItr != currentArgItr->cend(); ++flagsItr)
                {
                    String argName = "-" + String(1, *flagsItr);
                    // Insert each flags separately prefixed with `-`
                    addCmdArg(argName
                        , ArgElementsRange
                        {
                            .argIdx = int32(currentArgItr - cmdLineElements.cbegin()),
                            .argValueIdx = -1,
                            .count = int32(flagsItr - currentArgItr->cbegin())
                        });
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
    const ProgramCmdLineInstance* thisInst = static_cast<const ProgramCmdLineInstance*>(this);
    auto itrHelp = thisInst->cmdArgs.find("--help");
    if (itrHelp == thisInst->cmdArgs.cend())
    {
        itrHelp = thisInst->cmdArgs.find("-h");
    }
    if (itrHelp != thisInst->cmdArgs.cend())
    {
        String outHelp = programDescription + "\n";
        for (const auto& allowedArg : thisInst->allowedArgs)
        {
            if (allowedArg.shortArgName.empty())
            {
                outHelp += StringFormat::format("\n\"%s\" - %s", allowedArg.argName, allowedArg.argDescription);
            }
            else
            {
                outHelp += StringFormat::format("\n\"%s\", \"%s\" - %s", allowedArg.argName, allowedArg.shortArgName, allowedArg.argDescription);
            }
        }
        Logger::log("CmdLineHelp", "\n%s\n", outHelp);
        return true;
    }
    return false;
}

void ProgramCmdLine::printCommandLine() const
{
    String appName;
    FileSystemFunctions::applicationDirectory(appName);

    String cmdLine;
    for (uint32 i = 1; i < cmdLineElements.size(); ++i)
    {
        cmdLine += " " + String(cmdLineElements[i]);
    }
    Logger::log("CommandLine", "%s%s", appName, cmdLine);
}

bool ProgramCmdLine::hasArg(const String& argName) const
{
    const ProgramCmdLineInstance* thisInst = static_cast<const ProgramCmdLineInstance*>(this);
    auto argItr = thisInst->cmdArgs.find(argName);
    return thisInst->cmdArgs.cend() != argItr;
}

bool ProgramCmdLine::getArg(String& outValue, const String& argName) const
{
    const ProgramCmdLineInstance* thisInst = static_cast<const ProgramCmdLineInstance*>(this);
    auto argItr = thisInst->cmdArgs.find(argName);
    if (thisInst->cmdArgs.cend() != argItr && argItr->second.count > 0)
    {
        outValue = cmdLineElements[argItr->second.argValueIdx];
        return true;
    }
    return false;
}

bool ProgramCmdLine::getArg(std::vector<String>& outValues, const String& argName) const
{
    const ProgramCmdLineInstance* thisInst = static_cast<const ProgramCmdLineInstance*>(this);
    auto argItr = thisInst->cmdArgs.find(argName);
    if (thisInst->cmdArgs.cend() != argItr && argItr->second.count > 0)
    {
        outValues.insert(outValues.begin(),
            cmdLineElements.cbegin() + argItr->second.argValueIdx
            , cmdLineElements.cbegin() + argItr->second.argValueIdx + argItr->second.count);
        return true;
    }
    return false;
}

String ProgramCmdLine::atIdx(uint32 idx) const
{
    fatalAssert(cmdLineElements.size() > idx, "%s() : Cmd line value idx %d out of range %lu", __func__, idx, cmdLineElements.size());
    return cmdLineElements[idx];
}

uint32 ProgramCmdLine::cmdLineCount() const
{
    return uint32(cmdLineElements.size());
}

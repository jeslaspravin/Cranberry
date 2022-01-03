#pragma once
#include "Types/CoreTypes.h"
#include "String/String.h"
#include "ProgramCoreExports.h"

struct PROGRAMCORE_EXPORT CmdLineArgument
{
    // Short arg is for flags alone
    CmdLineArgument(String description, String cmdArg, String shortArg = "");
};

/*
* Args starting with `single -` will be short hand flags and can be stringed together and they cannot accept values
* Args starting with `--` Can have values space delimited strings
* Args starting with `@` refers to file which contains all arguments(It can be absolute path or relative to application directory)
* All args are case senstive
*/
class PROGRAMCORE_EXPORT ProgramCmdLine
{
private:
    // Args parsed from file are placed here
    String argsFromFile;
protected:
    ProgramCmdLine() = default;

    String programDescription;
    std::vector<std::string_view> cmdLineElements;
private:
    bool parseFromFile(const String& filePath);
public:
    static ProgramCmdLine* get();
    // Takes the pointer as it is and hold it for all parsing and data retrieval
    bool parse(AChar* cmdArgs[], uint32 count);
    // Takes the reference as it is and uses the AChar* to the cmdLine references
    // NOTE: The String passed in must live throughout the life time of ProgramCmdLine
    bool parse(const String& cmdLine);
    void setProgramDescription(const String& description)
    {
        programDescription = description;
    }

    // Prints help if the command line has asked for it and returns true if this cmd is invoked to print help.
    // Must be called after parsing the cmd line
    bool printHelp() const;
    void printCommandLine() const;

    // Must be queried with `-` or `--`
    bool hasArg(const String& argName) const;
    bool getArg(String& outValue, const String& argName) const;
    bool getArg(std::vector<String>& outValues, const String& argName) const;

    // Gets command line value at index
    String atIdx(uint32 idx) const;
    uint32 cmdLineCount() const;
};
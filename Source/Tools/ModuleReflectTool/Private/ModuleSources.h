/*!
 * \file ModuleSources.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once
#include "String/String.h"

#include <clang-c/Index.h>

class FileChangesTracker;

struct SourceInformation
{
    String filePath;
    String headerIncl;
    String generatedHeaderPath;
    String generatedTUPath;
    // TU will be parsed and be valid only if this source's reflection data is outdated. 
    // Failing to parse will lead to termination. So this must be valid if this source is parsed
    CXTranslationUnit tu = nullptr;
    // File size used for sorting
    uint32 fileSize = 0;
};

/*
* This class processes each of the header in the module.
*/
class ModuleSources
{
private:
    std::vector<String> genFiles;
    std::vector<String> includes;
    std::vector<String> compileDefs;
    String intermediateDir;
    String genDir;
    String srcDir;

    FileChangesTracker *headerTracker;
    CXIndex index;
    std::vector<SourceInformation> sources;
private:
    void printDiagnostics(CXDiagnostic diagnostic, uint32 formatOptions, uint32 idx);
    void addAdditionalCompileOpts(std::vector<std::string>& compilerArgs);
public:
    ModuleSources();
    ~ModuleSources();

    bool compileAllSources(bool bFullCompile = false);
    // Injects generated TU's into generate module files for build system to compile
    void injectGeneratedFiles(const std::vector<const SourceInformation*>& generatedSrcs);

    std::vector<const SourceInformation*> getParsedSources() const;
};
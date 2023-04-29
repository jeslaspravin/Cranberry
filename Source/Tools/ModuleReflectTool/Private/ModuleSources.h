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

#include "ModuleReflectTypes.h"

class FileChangesTracker;

/*
 * This class processes each of the header in the module.
 */
class ModuleSources
{
private:
    std::vector<String> genFiles;
    std::vector<String> includes;
    std::vector<String> compileDefs;
    std::vector<String> depIntermDirs;
    String intermediateDir;
    String genDir;
    String srcDir;
    String reflectedTypesFile;

    FileChangesTracker *headerTracker;
    CXIndex index;
    std::vector<SourceInformation> sources;

private:
    void printDiagnostics(CXDiagnostic diagnostic, uint32 formatOptions, uint32 idx);
    void addAdditionalCompileOpts(std::vector<std::string> &compilerArgs);
    // Deleted all generated files for each header
    void clearGenerated(const std::vector<String> &headers) const;

public:
    ModuleSources();
    ~ModuleSources();

    bool compileAllSources(bool bFullCompile = false);
    // Injects generated TU's into generate module files for build system to compile
    void injectGeneratedFiles(const std::vector<const SourceInformation *> &generatedSrcs, std::vector<ReflectedTypeItem> moduleReflectedTypes);

    std::vector<const SourceInformation *> getParsedSources() const;
    std::vector<ReflectedTypeItem> getDepReflectedTypes() const;
    std::vector<ReflectedTypeItem> getModuleReflectedTypes() const;
};
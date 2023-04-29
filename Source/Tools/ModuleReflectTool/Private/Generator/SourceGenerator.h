/*!
 * \file SourceGenerator.h
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

class ModuleSources;
class SourceGenerator;

class SourceGenerator
{
private:
    // For each source there will be one entry here to hold all the context necessary to generate
    // reflection header and source
    std::unordered_map<const SourceInformation *, SourceGeneratorContext> sourceToGenCntxt;
    std::unordered_set<ReflectedTypeItem> allKnownReflectedTypes;
    String moduleName;
    // Temporary will be valid only until parseSources() starts, Contains previously know module reflected files
    std::vector<ReflectedTypeItem> moduleReflectedTypes;

    // If any generation failed then this will be true, This flags is to continue generating even when
    // some sources fails
    bool bHasAnyError = false;
    FORCE_INLINE static std::vector<String> getTemplateFiles();

public:
    void initialize(const ModuleSources *sources);
    void parseSources();
    void writeGeneratedFiles();
    // Return true if no error
    bool generatedSources(std::vector<const SourceInformation *> &outGeneratedSrcs) const;
    const std::unordered_set<ReflectedTypeItem> &getKnownReflectedTypes() const { return allKnownReflectedTypes; }
    bool isFromCurrentModule(const ReflectedTypeItem &reflectItem) const { return reflectItem.moduleName == moduleName; }
    static bool isTemplatesModified();
};

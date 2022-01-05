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
#include "String/MustacheFormatString.h"

#include <unordered_set>

struct SourceInformation;
class ModuleSources;

struct SourceGeneratorContext
{
    std::vector<MustacheContext> headerReflectTypes;
    std::vector<MustacheContext> allRegisteredypes;
    std::vector<MustacheContext> qualifiedTypes;
    std::vector<MustacheContext> pairTypes;
    std::vector<MustacheContext> containerTypes;
    std::vector<MustacheContext> mapTypes;
    std::vector<MustacheContext> enumTypes;
    std::vector<MustacheContext> classTypes;
    // List of symbols that are adding in this generated TU
    std::unordered_set<String> addedSymbols;
    // If any error it will be set false
    bool bGenerated = true;
};

class SourceGenerator
{    
private:
    // For each source there will be one entry here to hold all the context necessary to generate reflection header and source
    std::unordered_map<const SourceInformation*, SourceGeneratorContext> sourceToGenCntxt;
    // If any generation failed then this will be true, This flags is to continue generating even when some sources fails
    bool bHasAnyError = false;
public:
    void initialize(const ModuleSources* sources);
    void parseSources();
    void writeGeneratedFiles();
    // Return true if no error
    bool generatedSources(std::vector<const SourceInformation*>& outGeneratedSrcs) const;
};

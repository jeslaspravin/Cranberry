/*!
 * \file SourceGenerator.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "Generator/SourceGenerator.h"
#include "CmdLine/CmdLine.h"
#include "CmdLineArgConst.h"
#include "Types/Platform/LFS/File/FileHelper.h"
#include "Types/Platform/LFS/PlatformLFS.h"
#include "ModuleSources.h"
#include "GeneratorConsts.h"
#include "Property/PropertyHelper.h"

void SourceGenerator::initialize(const ModuleSources* sources)
{
    std::vector<const SourceInformation*> parsedSrcs = sources->getParsedSources();
    sourceToGenCntxt.clear();
    sourceToGenCntxt.reserve(parsedSrcs.size());
    for (const SourceInformation* srcInfo : parsedSrcs)
    {
        sourceToGenCntxt.insert({ srcInfo, {} });
    }
}

void SourceGenerator::writeGeneratedFiles()
{
    std::vector<String> templateFiles = FileSystemFunctions::listFiles(TCHAR(TEMPLATES_DIR), true, TCHAR("*.mustache"));
    std::unordered_map<String, MustacheStringFormatter> sourceGenTemplates;
    sourceGenTemplates.reserve(templateFiles.size());
    for (const String& filePath : templateFiles)
    {
        String fileContent;
        if (FileHelper::readString(fileContent, filePath) && !fileContent.empty())
        {
            sourceGenTemplates.insert(
                { 
                    PathFunctions::stripExtension(PathFunctions::fileOrDirectoryName(filePath))
                    , MustacheStringFormatter(fileContent) 
                });
        }
    }
    String moduleExpMacro;
    ProgramCmdLine::get()->getArg(moduleExpMacro, ReflectToolCmdLineConst::MODULE_EXP_MACRO);

    for (const auto& sourceGen : sourceToGenCntxt)
    {
        const SourceInformation* srcInfo = sourceGen.first;
        const SourceGeneratorContext& srcGenCntxt = sourceGen.second;
        if (!srcGenCntxt.bGenerated)
        {
            continue;
        }

        String headerFileID = PropertyHelper::getValidSymbolName(srcInfo->headerIncl);

        // Generate header file
        MustacheContext headerContext;
        headerContext.args[GeneratorConsts::HEADERFILEID_TAG] = headerFileID;
        headerContext.sectionContexts[GeneratorConsts::REFLECTTYPES_SECTION_TAG] = srcGenCntxt.headerReflectTypes;
        headerContext.args[GeneratorConsts::EXPORT_SYMBOL_MACRO] = moduleExpMacro;

        String headerContent = sourceGenTemplates[GeneratorConsts::REFLECTHEADER_TEMPLATE].render(headerContext, sourceGenTemplates);
        if (!FileHelper::writeString(headerContent, srcInfo->generatedHeaderPath))
        {
            LOG_ERROR("SourceGenerator", "%s() : Could not write generated header(%s) for header %s", __func__, srcInfo->generatedHeaderPath, srcInfo->headerIncl);
            bHasAnyError = true;
            continue;
        }

        // Generate source file
        MustacheContext sourceContext;
        sourceContext.args[GeneratorConsts::HEADERFILEID_TAG] = headerFileID;
        sourceContext.args[GeneratorConsts::INCLUDEHEADER_TAG] = srcInfo->headerIncl;
        sourceContext.sectionContexts[GeneratorConsts::ALLREGISTERTYPES_SECTION_TAG] = srcGenCntxt.allRegisteredypes;
        sourceContext.sectionContexts[GeneratorConsts::QUALIFIEDTYPES_SECTION_TAG] = srcGenCntxt.qualifiedTypes;
        sourceContext.sectionContexts[GeneratorConsts::PAIRTYPES_SECTION_TAG] = srcGenCntxt.pairTypes;
        sourceContext.sectionContexts[GeneratorConsts::MAPTYPES_SECTION_TAG] = srcGenCntxt.mapTypes;
        sourceContext.sectionContexts[GeneratorConsts::CONTAINERTYPES_SECTION_TAG] = srcGenCntxt.containerTypes;
        sourceContext.sectionContexts[GeneratorConsts::ENUMTYPES_SECTION_TAG] = srcGenCntxt.enumTypes;
        sourceContext.sectionContexts[GeneratorConsts::CLASSTYPES_SECTION_TAG] = srcGenCntxt.classTypes;

        String sourceContent = sourceGenTemplates[GeneratorConsts::REFLECTSOURCE_TEMPLATE].render(sourceContext, sourceGenTemplates);
        if (!FileHelper::writeString(sourceContent, srcInfo->generatedTUPath))
        {
            LOG_ERROR("SourceGenerator", "%s() : Could not write generated sources(%s) for header %s", __func__, srcInfo->generatedTUPath, srcInfo->headerIncl);
            bHasAnyError = true;
            continue;
        }
    }
}

bool SourceGenerator::generatedSources(std::vector<const SourceInformation*>& outGeneratedSrcs) const
{
    outGeneratedSrcs.reserve(sourceToGenCntxt.size());
    bool bAnyGenFailure = false;
    for (const auto& sourceGen : sourceToGenCntxt)
    {
        bAnyGenFailure = bAnyGenFailure || !sourceGen.second.bGenerated;
        if (sourceGen.second.bGenerated)
        {
            outGeneratedSrcs.emplace_back(sourceGen.first);
        }
    }
    return !(bHasAnyError || bAnyGenFailure);
}
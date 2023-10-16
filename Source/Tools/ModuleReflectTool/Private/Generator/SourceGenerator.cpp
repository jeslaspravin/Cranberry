/*!
 * \file SourceGenerator.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "Generator/SourceGenerator.h"
#include "CmdLine/CmdLine.h"
#include "CmdLineArgConst.h"
#include "GeneratorConsts.h"
#include "ModuleSources.h"
#include "Property/PropertyHelper.h"
#include "Types/Platform/LFS/File/FileHelper.h"
#include "Types/Platform/LFS/PlatformLFS.h"
#include "Types/Platform/LFS/PathFunctions.h"
#include "Types/Platform/Threading/CoPaT/DispatchHelpers.h"
#include "Types/Platform/Threading/CoPaT/JobSystemCoroutine.h"
#include "Types/Platform/Threading/CoPaT/CoroutineWait.h"
#include "Types/Platform/Threading/CoPaT/CoroutineAwaitAll.h"

// "(\\\r?\n)\1+" to "$1"
// Replaces all extra \\r\n and compresses the generated code
const StringRegex hCompressRegex{ TCHAR("(\\\\\\r?\\n)\\1+"), std::regex_constants::ECMAScript };
const TChar *hCompressFmt = TCHAR("$1");

// " *(\r?\n)(?: *\1){2,}" to "$1$1"
// Replaces all extra \r\n, \t and compresses the generated code
const StringRegex cppCompressRegex{ TCHAR(" *(\\r?\\n)(?: *\\1){2,}"), std::regex_constants::ECMAScript };
const TChar *cppCompressFmt = TCHAR("$1$1");

FORCE_INLINE std::vector<String> SourceGenerator::getTemplateFiles()
{
    return FileSystemFunctions::listFiles(TCHAR(TEMPLATES_DIR), true, TCHAR("*.mustache"));
}

void SourceGenerator::initialize(const ModuleSources *sources)
{
    bHasAnyError.store(false, std::memory_order::relaxed);

    std::vector<const SourceInformation *> parsedSrcs = sources->getParsedSources();
    sourceGenCntxts.clear();
    sourceGenCntxts.reserve(parsedSrcs.size());
    for (const SourceInformation *srcInfo : parsedSrcs)
    {
        sourceGenCntxts.emplace_back(SourceGeneratorContext{ .generator = this }, srcInfo);
    }

    std::vector<ReflectedTypeItem> allReflectTypes = sources->getDepReflectedTypes();
    allKnownReflectedTypes.insert(allReflectTypes.cbegin(), allReflectTypes.cend());
    moduleReflectedTypes = sources->getModuleReflectedTypes();

    ProgramCmdLine::get().getArg(moduleName, ReflectToolCmdLineConst::MODULE_NAME);
}

void SourceGenerator::writeGeneratedFiles()
{
    std::vector<String> templateFiles = getTemplateFiles();
    std::unordered_map<String, MustacheStringFormatter> sourceGenTemplates;
    sourceGenTemplates.reserve(templateFiles.size());
    for (const String &filePath : templateFiles)
    {
        String fileContent;
        if (FileHelper::readString(fileContent, filePath) && !fileContent.empty())
        {
            sourceGenTemplates.insert({ PathFunctions::stripExtension(PathFunctions::fileOrDirectoryName(filePath)),
                                        MustacheStringFormatter(fileContent) });
        }
    }
    String moduleExpMacro;
    ProgramCmdLine::get().getArg(moduleExpMacro, ReflectToolCmdLineConst::MODULE_EXP_MACRO);

    copat::parallelFor(
        copat::JobSystem::get(),
        copat::DispatchFunctionType::createLambda(
            [this, &sourceGenTemplates, &moduleExpMacro](uint32 idx)
            {
                const SourceInformation *srcInfo = sourceGenCntxts[idx].second;
                const SourceGeneratorContext &srcGenCntxt = sourceGenCntxts[idx].first;
                if (!srcGenCntxt.bGenerated)
                {
                    return;
                }

                String headerFileID = PropertyHelper::getValidSymbolName(srcInfo->headerIncl);
                String headerFileName = PathFunctions::stripExtension(PathFunctions::fileOrDirectoryName(srcInfo->headerIncl));

                // Generate header file
                MustacheContext headerContext;
                headerContext.args[GeneratorConsts::HEADERFILEID_TAG] = headerFileID;
                headerContext.sectionContexts[GeneratorConsts::REFLECTTYPES_SECTION_TAG] = srcGenCntxt.headerReflectTypes;
                headerContext.args[GeneratorConsts::EXPORT_SYMBOL_MACRO] = moduleExpMacro;

                String headerContent = sourceGenTemplates[GeneratorConsts::REFLECTHEADER_TEMPLATE].render(headerContext, sourceGenTemplates);
                headerContent = std::regex_replace(headerContent, hCompressRegex, hCompressFmt);
                if (!FileHelper::writeString(headerContent, srcInfo->generatedHeaderPath))
                {
                    LOG_ERROR(
                        "SourceGenerator", "Could not write generated header({}) for header {}", srcInfo->generatedHeaderPath,
                        srcInfo->headerIncl
                    );
                    bHasAnyError.store(true, std::memory_order::relaxed);
                    return;
                }

                // Generate source file
                MustacheContext sourceContext;
                sourceContext.args[GeneratorConsts::REFLECTIONTUDEF_TAG] = headerFileName.toUpperCopy() + TCHAR("_GEN_TU");
                sourceContext.args[GeneratorConsts::HEADERFILEID_TAG] = headerFileID;
                sourceContext.args[GeneratorConsts::INCLUDEHEADER_TAG] = srcInfo->headerIncl;
                sourceContext.sectionContexts[GeneratorConsts::ADDITIONALINCLUDES_SECTION_TAG] = srcGenCntxt.additionalIncludes;
                sourceContext.sectionContexts[GeneratorConsts::ALLREGISTERTYPES_SECTION_TAG] = srcGenCntxt.allRegisteredypes;
                sourceContext.sectionContexts[GeneratorConsts::QUALIFIEDTYPES_SECTION_TAG] = srcGenCntxt.qualifiedTypes;
                sourceContext.sectionContexts[GeneratorConsts::PAIRTYPES_SECTION_TAG] = srcGenCntxt.pairTypes;
                sourceContext.sectionContexts[GeneratorConsts::MAPTYPES_SECTION_TAG] = srcGenCntxt.mapTypes;
                sourceContext.sectionContexts[GeneratorConsts::CONTAINERTYPES_SECTION_TAG] = srcGenCntxt.containerTypes;
                sourceContext.sectionContexts[GeneratorConsts::ENUMTYPES_SECTION_TAG] = srcGenCntxt.enumTypes;
                sourceContext.sectionContexts[GeneratorConsts::CLASSTYPES_SECTION_TAG] = srcGenCntxt.classTypes;

                String sourceContent = sourceGenTemplates[GeneratorConsts::REFLECTSOURCE_TEMPLATE].render(sourceContext, sourceGenTemplates);
                sourceContent = std::regex_replace(sourceContent, cppCompressRegex, cppCompressFmt);
                if (!FileHelper::writeString(sourceContent, srcInfo->generatedTUPath))
                {
                    LOG_ERROR(
                        "SourceGenerator", "Could not write generated sources({}) for header {}", srcInfo->generatedTUPath, srcInfo->headerIncl
                    );
                    bHasAnyError.store(true, std::memory_order::relaxed);
                    return;
                }
            }
        ),
        uint32(sourceGenCntxts.size())
    );
}

bool SourceGenerator::generatedSources(std::vector<const SourceInformation *> &outGeneratedSrcs) const
{
    outGeneratedSrcs.reserve(sourceGenCntxts.size());
    bool bAnyGenFailure = false;
    for (const auto &sourceGen : sourceGenCntxts)
    {
        const SourceInformation *srcInfo = sourceGen.second;
        const SourceGeneratorContext &srcGenCntxt = sourceGen.first;

        bAnyGenFailure = bAnyGenFailure || !srcGenCntxt.bGenerated;
        if (srcGenCntxt.bGenerated)
        {
            outGeneratedSrcs.emplace_back(srcInfo);
        }
    }
    return !(bHasAnyError.load(std::memory_order::relaxed) || bAnyGenFailure);
}

bool SourceGenerator::issueFullRecompile()
{
    String fullReflectTs;
    ProgramCmdLine::get().getArg(fullReflectTs, ReflectToolCmdLineConst::INTERMEDIATE_DIR);
    fullReflectTs = PathFunctions::combinePath(fullReflectTs, "FullReflect.timestamp");
    PlatformFile fullReflectTsFile(fullReflectTs);
    if (!fullReflectTsFile.exists())
    {
        FileHelper::touchFile(fullReflectTs);
        return true;
    }

    // Check template files
    std::vector<String> templateFiles = getTemplateFiles();
    TickRep lastModifiedTs = 0;
    for (const String &templateFile : templateFiles)
    {
        TickRep ts = PlatformFile(templateFile).lastWriteTimeStamp();
        lastModifiedTs = lastModifiedTs < ts ? ts : lastModifiedTs;
    }

    // Check if ModuleReflectTool is new
    TickRep reflectToolTs = PlatformFile(FileSystemFunctions::applicationPath()).lastWriteTimeStamp();
    lastModifiedTs = lastModifiedTs < reflectToolTs ? reflectToolTs : lastModifiedTs;

    // If last modified after last generation
    if (fullReflectTsFile.lastWriteTimeStamp() < lastModifiedTs)
    {
        FileHelper::touchFile(fullReflectTs);
        return true;
    }
    return false;
}

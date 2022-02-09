/*!
 * \file ModuleSources.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "ModuleSources.h"
#include "CmdLine/CmdLine.h"
#include "Logger/Logger.h"
#include "Types/Platform/LFS/File/FileHelper.h"
#include "Types/Platform/LFS/PlatformLFS.h"
#include "FileChangesTracker.h"
#include "Parser/ParserHelper.h"
#include "Parser/ClangWrappers.h"
#include "CmdLineArgConst.h"
#include "String/StringRegex.h"

#include "SampleCode.h"



void ModuleSources::printDiagnostics(CXDiagnostic diagnostic, uint32 formatOptions, uint32 idx)
{
    CXDiagnosticSet childDiags = clang_getChildDiagnostics(diagnostic);
    const uint32 childDiagsNum = clang_getNumDiagnosticsInSet(childDiags);

    String diagnosticStr(CXStringWrapper(clang_formatDiagnostic(diagnostic, formatOptions)).toString());
    // Ignore include failed on gen.h files
    static const StringRegex inclGenHeaderMatch(TCHAR(".*'.*.gen.h' file not found.*"), std::regex_constants::ECMAScript);
    if (std::regex_match(diagnosticStr, inclGenHeaderMatch))
    {
        return;
    }

    LOG_WARN("Diagnostics", "[%d]%s", idx, diagnosticStr);
    for (int32 i = 0; i < childDiagsNum; ++i)
    {
        CXDiagnostic childDiagnostic = clang_getDiagnosticInSet(childDiags, i);
        printDiagnostics(childDiagnostic, formatOptions, idx);
        clang_disposeDiagnostic(childDiagnostic);
    }
}

void ModuleSources::addAdditionalCompileOpts(std::vector<std::string>& compilerArgs)
{
    if (!ProgramCmdLine::get()->hasArg(ReflectToolCmdLineConst::FILTER_DIAGNOSTICS))
    {
        return;
    }

    compilerArgs.emplace_back("-Wno-ignored-attributes");
}

ModuleSources::ModuleSources()
    : headerTracker(nullptr)
    , index(nullptr)
{
    String includesFile;
    String compileDefsFile;
    ProgramCmdLine::get()->getArg(genFiles, ReflectToolCmdLineConst::GENERATED_TU_LIST);
    ProgramCmdLine::get()->getArg(srcDir, ReflectToolCmdLineConst::MODULE_SRC_DIR);
    ProgramCmdLine::get()->getArg(genDir, ReflectToolCmdLineConst::GENERATED_DIR);
    ProgramCmdLine::get()->getArg(intermediateDir, ReflectToolCmdLineConst::INTERMEDIATE_DIR);
    ProgramCmdLine::get()->getArg(includesFile, ReflectToolCmdLineConst::INCLUDE_LIST_FILE);
    ProgramCmdLine::get()->getArg(compileDefsFile, ReflectToolCmdLineConst::COMPILE_DEF_LIST_FILE);
    LOG_DEBUG("ModuleReflectTool", "%s(): Reflecting source from %s", __func__, srcDir);

    fatalAssert((PlatformFile(includesFile).exists() && PlatformFile(compileDefsFile).exists())
        , "%s() : Includes list file(%s) or Definitions(%s) list file does not exists, Configuring cmake will fix this!", __func__, includesFile, compileDefsFile);

    String content;
    // Read includes file
    if (!FileHelper::readString(content, includesFile))
    {
        fatalAssert(!"Failed to read include file", "Failed to read include file from %s", includesFile);
    }
    for (const String& includeList : content.splitLines())
    {
        std::vector<String> includeFolders = String::split(includeList, TCHAR(";"));
        for (const String& includeFolder : includeFolders)
        {
            includes.emplace_back(includeFolder.trimCopy());
        }
    }

    content.clear();
    // Read compile defines
    if (!FileHelper::readString(content, compileDefsFile))
    {
        fatalAssert(!"Failed to read compile definitions", "Failed to read compile definitions from %s", compileDefsFile);
    }
    // Parsing compile definitions
    for (const String& compileDefsList : content.splitLines())
    {
        // We process defines a little different than includes since we have ; separated values in defines as well inside qoutes
        auto beginTokenItr = compileDefsList.cbegin();
        for (auto tokenItr = compileDefsList.cbegin(); tokenItr != compileDefsList.cend(); ++tokenItr)
        {
            // We do not allow quoted value inside a quote, so there will be only single level of quote in compile definitions
            if ((*tokenItr) == '"')
            {
                ++tokenItr;
                while (tokenItr != compileDefsList.cend() && (*tokenItr != '"'))
                {
                    ++tokenItr;
                }
            }
            // If semicolon outside quote then add it as compile definition
            else if((*tokenItr) == ';')
            {
                // If not consecutive ; token then add
                if (beginTokenItr != tokenItr)
                {
                    compileDefs.emplace_back(String(beginTokenItr, tokenItr));
                }
                // Next token must be new begining skipping ;
                beginTokenItr = tokenItr + 1;
            }
        }
    }

    headerTracker = new FileChangesTracker(PathFunctions::fileOrDirectoryName(srcDir), srcDir, intermediateDir);
}

ModuleSources::~ModuleSources()
{
    delete headerTracker;
    headerTracker = nullptr;

    for (const SourceInformation& srcInfo : sources)
    {
        clang_disposeTranslationUnit(srcInfo.tu);
    }
    if (index)
    {
        clang_disposeIndex(index);
    }
}

bool ModuleSources::compileAllSources()
{
    bool bAllClear = true;
    std::vector<String> headerFiles = FileSystemFunctions::listFiles(srcDir, true, TCHAR("*.h"));
    // Update to current header lists
    headerTracker->intersectFiles(headerFiles);
    sources.reserve(headerFiles.size());

    if (headerFiles.empty())
    {
        return bAllClear;
    }
    String publicHeadersPath = PathFunctions::combinePath(srcDir, TCHAR("Public"));
    String privateHeadersPath = PathFunctions::combinePath(srcDir, TCHAR("Private"));

    // Create a common index for this module
    index = clang_createIndex(0, 0);
    std::vector<std::string> moduleArgs;
    moduleArgs.emplace_back("-std=c++20");
    moduleArgs.emplace_back("-D__REF_PARSE__");
    addAdditionalCompileOpts(moduleArgs);
    for (const String& compileDef : compileDefs)
    {
        moduleArgs.emplace_back("-D" + std::string(TCHAR_TO_ANSI(compileDef.getChar())));
    }
    for (const String& incl : includes)
    {
        moduleArgs.emplace_back("-I" + std::string(TCHAR_TO_ANSI(incl.getChar())));
    }
    std::vector<const AChar*> argsPtrs(moduleArgs.size());
    // Fill args pointers
    for (uint32 i = 0; i < moduleArgs.size(); ++i)
    {
        argsPtrs[i] = moduleArgs[i].c_str();
    }

    for (uint32 i = 0; i < headerFiles.size(); ++i)
    {
        if (!ParserHelper::shouldReflectHeader(headerFiles[i]))
        {
            continue;
        }
        PlatformFile headerFile(headerFiles[i]);

        SourceInformation& sourceInfo = sources.emplace_back();
        sourceInfo.filePath = headerFile.getFullPath();
        sourceInfo.fileSize = headerFile.fileSize();
        sourceInfo.generatedTUPath = PathFunctions::combinePath(genDir, TCHAR("Private"), PathFunctions::stripExtension(headerFile.getFileName()) + TCHAR(".gen.cpp"));
        // Generated header needs to be in public as well for public headers
        if (PathFunctions::isSubdirectory(sourceInfo.filePath, publicHeadersPath))
        {
            sourceInfo.generatedHeaderPath = PathFunctions::combinePath(genDir, TCHAR("Public"), PathFunctions::stripExtension(headerFile.getFileName()) + TCHAR(".gen.h"));
            sourceInfo.headerIncl = PathFunctions::toRelativePath(sourceInfo.filePath, publicHeadersPath);
        }
        else
        {
            // This private header case happens for file which are in Private folder of srcDir or ones in srcDir itself, only files in Public folder is public
            sourceInfo.generatedHeaderPath = PathFunctions::combinePath(genDir, TCHAR("Private"), PathFunctions::stripExtension(headerFile.getFileName()) + TCHAR(".gen.h"));
            if (PathFunctions::isSubdirectory(sourceInfo.filePath, privateHeadersPath))
            {
                sourceInfo.headerIncl = PathFunctions::toRelativePath(sourceInfo.filePath, privateHeadersPath);
            }
            else
            {
                sourceInfo.headerIncl = PathFunctions::toRelativePath(sourceInfo.filePath, srcDir);
            }
        }

        // If output is no longer valid to current input file regenerate reflection
        if (headerTracker->isTargetOutdated(sourceInfo.filePath, { sourceInfo.generatedHeaderPath, sourceInfo.generatedTUPath }))
        {
            // Use parse TU functions if need to customize certain options while compiling
            // Header.H - H has to be capital but why?
            // It is okay if we miss some insignificant includes as they are ignored and parsing continues
            String headerPath = PathFunctions::combinePath(headerFile.getHostDirectory(), PathFunctions::stripExtension(headerFile.getFileName()) + TCHAR(".H"));
            CXTranslationUnit unit = clang_parseTranslationUnit(
                index,
                TCHAR_TO_ANSI(headerPath.getChar())
                , argsPtrs.data(), int32(argsPtrs.size()), nullptr, 0, CXTranslationUnit_KeepGoing);

            if (unit == nullptr)
            {
                LOG_ERROR("CompileSource", "%s() : Unable to parse header %s. Quitting.", __func__, headerFile.getFullPath());
                bAllClear = false;
            }
            else
            {
                if (!ProgramCmdLine::get()->hasArg(ReflectToolCmdLineConst::NO_DIAGNOSTICS))
                {
                    uint32 formatOptions = CXDiagnostic_DisplaySourceLocation | CXDiagnostic_DisplayColumn | CXDiagnostic_DisplayCategoryName | CXDiagnostic_DisplayOption;
                    uint32 diagnosticsNum = clang_getNumDiagnostics(unit);
                    for (uint32 i = 0; i < diagnosticsNum; ++i)
                    {
                        auto diagnostic = clang_getDiagnostic(unit, i);
                        printDiagnostics(diagnostic, formatOptions, i);
                        clang_disposeDiagnostic(diagnostic);
                    }
                }

                sourceInfo.tu = unit;
            }
        }
    }

    return bAllClear;
}

void ModuleSources::injectGeneratedFiles(const std::vector<const SourceInformation*>& generatedSrcs)
{
    std::sort(sources.begin(), sources.end(), [](const SourceInformation& lhs, const SourceInformation& rhs)
        {
            return lhs.fileSize > rhs.fileSize;
        });

    // For each gen files
    for (uint32 i = 0; i < genFiles.size(); ++i)
    {
        std::vector<String> includeStmts;
        // Since stride is number of generated module files
        for (uint32 j = 0; j < sources.size(); j += genFiles.size())
        {
            includeStmts.emplace_back(StringFormat::format(TCHAR("#include \"%s\"")
                , PathFunctions::fileOrDirectoryName(sources[j].generatedTUPath)));
        }

        String genFileContent = String::join(includeStmts.cbegin(), includeStmts.cend(), LINE_FEED_CHAR);
        if (!FileHelper::writeString(genFileContent, genFiles[i]))
        {
            LOG_ERROR("GeneratingBuildTU", "%s() : Failed to write file %s", __func__, genFiles[i]);
            std::exit(1);
            return;
        }
    }
    // Now generating is done so mark the tracker manifest with generated files
    for (uint32 i = 0; i < generatedSrcs.size(); ++i)
    {
        if (sources[i].tu != nullptr)
        {
            headerTracker->updateNewerFile(generatedSrcs[i]->filePath, { generatedSrcs[i]->generatedHeaderPath, generatedSrcs[i]->generatedTUPath });
        }
    }
}

std::vector<const SourceInformation*> ModuleSources::getParsedSources() const
{
    std::vector<const SourceInformation*> retVal;
    for (uint32 i = 0; i < sources.size(); ++i)
    {
        if (sources[i].tu != nullptr)
        {
            retVal.emplace_back(&sources[i]);
        }
    }
    return retVal;
}
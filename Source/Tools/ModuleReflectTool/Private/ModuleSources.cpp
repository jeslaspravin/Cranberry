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
#include "Types/Platform/LFS/PlatformLFS.h"
#include "FileChangesTracker.h"
#include "Parser/ParserHelper.h"
#include "Parser/ClangWrappers.h"
#include "CmdLineArgConst.h"

#include "SampleCode.h"
#include <regex>



void ModuleSources::printDiagnostics(CXDiagnostic diagnostic, uint32 formatOptions, uint32 idx)
{
    CXDiagnosticSet childDiags = clang_getChildDiagnostics(diagnostic);
    const uint32 childDiagsNum = clang_getNumDiagnosticsInSet(childDiags);

    String diagnosticStr(CXStringWrapper(clang_formatDiagnostic(diagnostic, formatOptions)).toString());
    // Ignore include failed on gen.h files
    static const std::regex inclGenHeaderMatch(".*'.*.gen.h' file not found.*", std::regex_constants::ECMAScript);
    if (std::regex_match(diagnosticStr, inclGenHeaderMatch))
    {
        return;
    }

    Logger::warn("Diagnostics", "[%d]%s", idx, diagnosticStr);
    for (int32 i = 0; i < childDiagsNum; ++i)
    {
        CXDiagnostic childDiagnostic = clang_getDiagnosticInSet(childDiags, i);
        printDiagnostics(childDiagnostic, formatOptions, idx);
        clang_disposeDiagnostic(childDiagnostic);
    }
}

void ModuleSources::addAdditionalCompileOpts(std::vector<String>& compilerArgs)
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
    Logger::debug("ModuleReflectTool", "%s(): Reflecting source from %s", __func__, srcDir);

    fatalAssert((PlatformFile(includesFile).exists() && PlatformFile(compileDefsFile).exists())
        , "%s() : Includes list file or Definitions list file does not exists, Configuring cmake will fix this!", __func__);

    String content;
    // Read includes file
    PlatformFile readFile(includesFile);
    readFile.setFileFlags(EFileFlags::Read);
    readFile.setSharingMode(EFileSharing::ReadOnly);
    readFile.setCreationAction(EFileFlags::OpenExisting);
    readFile.openFile();
    readFile.read(content);
    readFile.closeFile();
    for (const String& includeList : content.splitLines())
    {
        std::vector<String> includeFolders = String::split(includeList, ";");
        for (const String& includeFolder : includeFolders)
        {
            includes.emplace_back(includeFolder.trimCopy());
        }
    }

    content.clear();
    // Read compile defines
    readFile = PlatformFile(compileDefsFile);
    readFile.setFileFlags(EFileFlags::Read);
    readFile.setSharingMode(EFileSharing::ReadOnly);
    readFile.setCreationAction(EFileFlags::OpenExisting);
    readFile.openFile();
    readFile.read(content);
    readFile.closeFile();
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

    headerTracker = new FileChangesTracker(PlatformFile(srcDir).getDirectoryName(), srcDir, intermediateDir);
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
    std::vector<String> headerFiles = FileSystemFunctions::listFiles(srcDir, true, "*.h");
    // Update to current header lists
    headerTracker->intersectFiles(headerFiles);
    sources.reserve(headerFiles.size());

    if (headerFiles.empty())
    {
        return bAllClear;
    }
    String publicHeadersPath = PathFunctions::combinePath(srcDir, "Public");
    String privateHeadersPath = PathFunctions::combinePath(srcDir, "Private");

    // Create a common index for this module
    index = clang_createIndex(0, 0);
    std::vector<String> moduleArgs;
    moduleArgs.emplace_back("-std=c++20");
    moduleArgs.emplace_back("-D__REF_PARSE__");
    addAdditionalCompileOpts(moduleArgs);
    for (const String& compileDef : compileDefs)
    {
        moduleArgs.emplace_back("-D" + compileDef);
    }
    for (const String& incl : includes)
    {
        moduleArgs.emplace_back("-I" + incl);
    }
    std::vector<const AChar*> argsPtrs(moduleArgs.size());
    // Fill args pointers
    for (uint32 i = 0; i < moduleArgs.size(); ++i)
    {
        argsPtrs[i] = moduleArgs[i].getChar();
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
        sourceInfo.generatedTUPath = PathFunctions::combinePath(genDir, "Private", PathFunctions::stripExtension(headerFile.getFileName()) + ".gen.cpp");
        // Generated header needs to be in public as well for public headers
        if (PathFunctions::isSubdirectory(sourceInfo.filePath, publicHeadersPath))
        {
            sourceInfo.generatedHeaderPath = PathFunctions::combinePath(genDir, "Public", PathFunctions::stripExtension(headerFile.getFileName()) + ".gen.h");
            sourceInfo.headerIncl = PathFunctions::toRelativePath(sourceInfo.filePath, publicHeadersPath);
        }
        else
        {
            // This private header case happens for file which are in Private folder of srcDir or ones in srcDir itself, only files in Public folder is public
            sourceInfo.generatedHeaderPath = PathFunctions::combinePath(genDir, "Private", PathFunctions::stripExtension(headerFile.getFileName()) + ".gen.h");
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
            String headerPath = PathFunctions::combinePath(headerFile.getHostDirectory(), PathFunctions::stripExtension(headerFile.getFileName()) + ".H");
            CXTranslationUnit unit = clang_parseTranslationUnit(
                index,
                headerPath.getChar()
                , argsPtrs.data(), int32(argsPtrs.size()), nullptr, 0, CXTranslationUnit_KeepGoing);

            if (unit == nullptr)
            {
                Logger::error("CompileSource", "%s() : Unable to parse header %s. Quitting.", __func__, headerFile.getFullPath());
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
            includeStmts.emplace_back(StringFormat::format("#include \"%s\""
                , PlatformFile(sources[j].generatedTUPath).getFileName()));
        }

        String genFileContent = String::join(includeStmts.cbegin(), includeStmts.cend(), LINE_FEED_CHAR);
        PlatformFile genFile(genFiles[i]);
        genFile.setFileFlags(EFileFlags::Write);
        genFile.setSharingMode(EFileSharing::ReadOnly);
        genFile.setCreationAction(EFileFlags::OpenAlways);
        if (!genFile.openOrCreate())
        {
            Logger::error("GeneratingBuildTU", "%s() : Failed to write file %s", __func__, genFile.getFullPath());
            std::exit(1);
            return;
        }
        genFile.write(ArrayView<uint8>(reinterpret_cast<uint8*>(genFileContent.data()), genFileContent.length()));
        genFile.closeFile();
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

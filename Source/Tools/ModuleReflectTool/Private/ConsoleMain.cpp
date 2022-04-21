/*!
 * \file ConsoleMain.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "CmdLine/CmdLine.h"
#include "CmdLineArgConst.h"
#include "Generator/SourceGenerator.h"
#include "Logger/Logger.h"
#include "Memory/Memory.h"
#include "ModuleSources.h"
#include "Modules/ModuleManager.h"
#include "Types/CoreTypes.h"
#include "Types/Platform/LFS/PathFunctions.h"

#include "SampleCode.h"

void initializeCmdArguments()
{
    CmdLineArgument genFilesList(
        TCHAR("List of file path that will be consumed by build as generated "
              "reflection translation units"),
        ReflectToolCmdLineConst::GENERATED_TU_LIST
    );
    CmdLineArgument generatedDirector(
        TCHAR("Directory where the generated files will be dropped.\n\
    Generated header for headers under Public folder, will be placed under public folder of this directory and others will be placed under Private\
    "),
        ReflectToolCmdLineConst::GENERATED_DIR
    );
    CmdLineArgument moduleSrcDir(
        TCHAR("Directory to search and parse source headers from for this module."), ReflectToolCmdLineConst::MODULE_SRC_DIR
    );
    CmdLineArgument moduleExpMacro(TCHAR("Name of API export macro for this module."), ReflectToolCmdLineConst::MODULE_EXP_MACRO);
    CmdLineArgument intermediateDir(
        TCHAR("Directory where intermediate files can be dropped/created.\n\
    This must be unique per configuration to track last generated timestamps for files etc,."),
        ReflectToolCmdLineConst::INTERMEDIATE_DIR
    );
    CmdLineArgument includeDirList(
        TCHAR("File path that contains list of include directories for this "
              "module semicolon(;) separated."),
        ReflectToolCmdLineConst::INCLUDE_LIST_FILE, TCHAR("--I")
    );
    CmdLineArgument compileDefList(
        TCHAR("File path that contains list of compile definitions for this "
              "module semicolon(;) separated."),
        ReflectToolCmdLineConst::COMPILE_DEF_LIST_FILE, TCHAR("--D")
    );
    CmdLineArgument exeSampleCode(TCHAR("Executes sample code instead of actual application"), ReflectToolCmdLineConst::SAMPLE_CODE);
    CmdLineArgument filterDiagnostics(
        TCHAR("Filters the diagnostics results and only display what is absolutely necessary"), ReflectToolCmdLineConst::FILTER_DIAGNOSTICS
    );
    CmdLineArgument noDiagnostics(TCHAR("No diagnostics will be displayed"), ReflectToolCmdLineConst::NO_DIAGNOSTICS);

    ProgramCmdLine::get()->setProgramDescription(TCHAR("ModuleReflectTool Copyright (C) Jeslas Pravin, Since 2022\n\
    Parses the headers in provided module and creates reflection files for them.\n\
    It uses clang libraries and mustache style templates to generate reflection data"));
}

// Override new and delete
CBE_GLOBAL_NEWDELETE_OVERRIDES

int32 main(int32 argsc, AChar **args)
{
    UnexpectedErrorHandler::getHandler()->registerFilter();
    Logger::pushMuteSeverities(Logger::Debug | Logger::Log);

    ModuleManager *moduleManager = ModuleManager::get();
    moduleManager->loadModule(TCHAR("ProgramCore"));
    initializeCmdArguments();

    if (!ProgramCmdLine::get()->parse(args, argsc))
    {
        LOG("CPPReflect", "%s(): Failed to parse command line arguments", __func__);
        ProgramCmdLine::get()->printCommandLine();
    }
    if (ProgramCmdLine::get()->printHelp())
    {
        // Since this invocation is for printing help
        return 0;
    }

    // Loading other libraries
    moduleManager->loadModule(TCHAR("ReflectionRuntime"));
    moduleManager->getOrLoadLibrary(
        PathFunctions::combinePath(TCHAR(LLVM_INSTALL_PATH), TCHAR("bin"), String(LIB_PREFIX) + TCHAR("libclang.") + SHARED_LIB_EXTENSION)
    );

    Logger::flushStream();

    if (ProgramCmdLine::get()->hasArg(ReflectToolCmdLineConst::SAMPLE_CODE))
    {
        LOG_DEBUG("CPPReflect", "%s(): Executing sample codes %s", __func__, ENGINE_MODULES_PATH);
        String srcDir = ProgramCmdLine::get()->atIdx(ProgramCmdLine::get()->cmdLineCount() - 1);

        SampleCode::testLibClangParsing(srcDir);
        // SampleCode::testTypesAndProperties();
        SampleCode::testPropertySystem();
        // SampleCode::testRegex();
        // SampleCode::testTemplateReflectionGeneration();
    }
    else
    {
        ModuleSources moduleSrcs;
        if (!moduleSrcs.compileAllSources(SourceGenerator::isTemplatesModified()))
        {
            LOG_ERROR("ModuleReflectTool", "%s() : Compiling module sources failed", __func__);
            return 1;
        }
        SourceGenerator generator;
        generator.initialize(&moduleSrcs);
        generator.parseSources();
        generator.writeGeneratedFiles();

        std::vector<const SourceInformation *> generatedSrcs;
        bool bGenerated = generator.generatedSources(generatedSrcs);
        moduleSrcs.injectGeneratedFiles(generatedSrcs);

        if (!bGenerated)
        {
            LOG_ERROR("ModuleReflectTool", "%s() : Generating module sources failed", __func__);
            return 1;
        }
    }

    moduleManager->unloadModule(TCHAR("ReflectionRuntime"));
    moduleManager->unloadModule(TCHAR("ProgramCore"));

    UnexpectedErrorHandler::getHandler()->unregisterFilter();
    Logger::flushStream();
    return 0;
}
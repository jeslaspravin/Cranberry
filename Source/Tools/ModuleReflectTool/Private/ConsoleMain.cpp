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


#include "Types/CoreTypes.h"
#include "Logger/Logger.h"
#include "Modules/ModuleManager.h"
#include "Types/Platform/LFS/PathFunctions.h"
#include "ModuleSources.h"
#include "Generator/SourceGenerator.h"
#include "CmdLineArgConst.h"
#include "CmdLine/CmdLine.h"

#include "SampleCode.h"

void initializeCmdArguments()
{
    CmdLineArgument genFilesList("List of file path that will be consumed by build as generated reflection translation units", ReflectToolCmdLineConst::GENERATED_TU_LIST);
    CmdLineArgument generatedDirector("Directory where the generated files will be dropped.\n\
    Generated header for headers under Public folder, will be placed under public folder of this directory and others will be placed under Private\
    ", ReflectToolCmdLineConst::GENERATED_DIR);
    CmdLineArgument moduleSrcDir("Directory to search and parse source headers from for this module.", ReflectToolCmdLineConst::MODULE_SRC_DIR);
    CmdLineArgument moduleExpMacro("Name of API export macro for this module.", ReflectToolCmdLineConst::MODULE_EXP_MACRO);
    CmdLineArgument intermediateDir("Directory where intermediate files can be dropped/created.\n\
    This must unique per configuration to track last generated timestamps for files etc,.", ReflectToolCmdLineConst::INTERMEDIATE_DIR);
    CmdLineArgument includeDirList("File path that contains list of include directories for this module semicolon(;) separated.", ReflectToolCmdLineConst::INCLUDE_LIST_FILE, "--I");
    CmdLineArgument compileDefList("File path that contains list of compile definitions for this module semicolon(;) separated.", ReflectToolCmdLineConst::COMPILE_DEF_LIST_FILE, "--D");
    CmdLineArgument exeSampleCode("Executes sample code instead of actual application", ReflectToolCmdLineConst::SAMPLE_CODE);
    CmdLineArgument filterDiagnostics("Filters the diagnostics results and only display what is absolutely necessary", ReflectToolCmdLineConst::FILTER_DIAGNOSTICS);
    CmdLineArgument noDiagnostics("No diagnostics will be displayed", ReflectToolCmdLineConst::NO_DIAGNOSTICS);

    ProgramCmdLine::get()->setProgramDescription("ModuleReflectTool Copyright (C) Jeslas Pravin, Since 2022\n\
    Parses the headers in provided module and creates reflection files for them.\n\
    It uses clang libraries and mustache style templates to generate reflection data");
}

int32 main(int32 argsc, AChar** args)
{
    UnexpectedErrorHandler::getHandler()->registerFilter();

    ModuleManager* moduleManager = ModuleManager::get();
    moduleManager->loadModule("ProgramCore");
    initializeCmdArguments();

    if (!ProgramCmdLine::get()->parse(args, argsc))
    {
        Logger::log("CPPReflect", "%s(): Failed to parse command line arguments", __func__);
    }
    if (ProgramCmdLine::get()->printHelp())
    {
        // Since this invocation is for printing help
        return 0;
    }
    ProgramCmdLine::get()->printCommandLine();

    // Loading other libraries
    moduleManager->loadModule("ReflectionRuntime");
    moduleManager->getOrLoadLibrary(PathFunctions::combinePath(LLVM_INSTALL_PATH, "bin", COMBINE(LIB_PREFIX, COMBINE("libclang.", SHARED_LIB_EXTENSION))));

    if (ProgramCmdLine::get()->hasArg(ReflectToolCmdLineConst::SAMPLE_CODE))
    {
        Logger::debug("CPPReflect", "%s(): Executing sample codes %s", __func__, ENGINE_MODULES_PATH);
        String srcDir = ProgramCmdLine::get()->atIdx(ProgramCmdLine::get()->cmdLineCount() - 1);

        SampleCode::testLibClangParsing(srcDir);
        //SampleCode::testTypesAndProperties();
        SampleCode::testPropertySystem();
        //SampleCode::testRegex();
        //SampleCode::testTemplateReflectionGeneration();
    }
    else
    {
        ModuleSources moduleSrcs;
        if (!moduleSrcs.compileAllSources())
        {
            Logger::error("ModuleReflectTool", "%s() : Compiling module sources failed", __func__);
            return 1;
        }
        SourceGenerator generator;
        generator.initialize(&moduleSrcs);
        generator.parseSources();
        generator.writeGeneratedFiles();

        std::vector<const SourceInformation*> generatedSrcs;
        bool bGenerated = generator.generatedSources(generatedSrcs);
        moduleSrcs.injectGeneratedFiles(generatedSrcs);

        if (!bGenerated)
        {
            Logger::error("ModuleReflectTool", "%s() : Generating module sources failed", __func__);
            return 1;
        }
    }

    moduleManager->unloadModule("ReflectionRuntime");
    moduleManager->unloadModule("ProgramCore");

    UnexpectedErrorHandler::getHandler()->unregisterFilter();
    Logger::flushStream();
    return 0;
}
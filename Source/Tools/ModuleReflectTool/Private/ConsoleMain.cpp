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
#include "Types/Platform/Threading/CoPaT/JobSystem.h"
#include "Types/Platform/LFS/PathFunctions.h"
#include "Types/Time.h"

#include "SampleCode.h"

void initializeCmdArguments()
{
    REGISTER_CMDARG(
        "List of file path that will be consumed by build as generated "
        "reflection translation units",
        ReflectToolCmdLineConst::GENERATED_TU_LIST.getChar()
    );
    REGISTER_CMDARG(
        "Directory where the generated files will be dropped.\n    "
        "Generated header for headers under Public folder, "
        "will be placed under public folder of this directory and others will be placed under Private",
        ReflectToolCmdLineConst::GENERATED_DIR.getChar()
    );
    REGISTER_CMDARG(
        "File where all the reflected types from this module must be written out.", ReflectToolCmdLineConst::REFLECTED_TYPES_LIST_FILE.getChar()
    );
    REGISTER_CMDARG("Directory to search and parse source headers from for this module.", ReflectToolCmdLineConst::MODULE_SRC_DIR.getChar());
    REGISTER_CMDARG(
        "Name of this module. This will be used to derive several build file names.", ReflectToolCmdLineConst::MODULE_NAME.getChar()
    );
    REGISTER_CMDARG("Name of API export macro for this module.", ReflectToolCmdLineConst::MODULE_EXP_MACRO.getChar());
    REGISTER_CMDARG(
        "Directory where intermediate files can be dropped/created.\n    "
        "This must be unique per configuration to track last generated timestamps for files etc,.",
        ReflectToolCmdLineConst::INTERMEDIATE_DIR.getChar()
    );
    REGISTER_CMDARG_S(
        "File path that contains list of include directories for this "
        "module semicolon(;) separated.",
        ReflectToolCmdLineConst::INCLUDE_LIST_FILE.getChar(),
        TCHAR("--I")
        );
    REGISTER_CMDARG_S(
        "File path that contains list of compile definitions for this "
        "module semicolon(;) separated.",
        ReflectToolCmdLineConst::COMPILE_DEF_LIST_FILE.getChar(),
        TCHAR("--D")
        );
    REGISTER_CMDARG(
        "Intermediate directories of the modules this module depends on.", ReflectToolCmdLineConst::DEP_INTERMEDIATE_DIRS_LIST_FILE.getChar()
    );
    REGISTER_CMDARG("Executes sample code instead of actual application", ReflectToolCmdLineConst::SAMPLE_CODE.getChar());
    REGISTER_CMDARG(
        "Filters the diagnostics results and only display what is absolutely necessary", ReflectToolCmdLineConst::FILTER_DIAGNOSTICS.getChar()
    );
    REGISTER_CMDARG("No diagnostics will be displayed", ReflectToolCmdLineConst::NO_DIAGNOSTICS.getChar());
    REGISTER_CMDARG("Sets the verbosity of logger to debug", ReflectToolCmdLineConst::LOG_VERBOSE.getChar());

    ProgramCmdLine::get().setProgramDescription(TCHAR(
    "ModuleReflectTool\nCopyright (C) Jeslas Pravin, Since 2022\n    "
    "Parses the headers in provided module and creates reflection files for them.\n    "
    "It uses clang libraries and mustache style templates to generate reflection data"
    ) );
}

// Override new and delete
CBE_GLOBAL_NEWDELETE_OVERRIDES

int32 main(int32 argsc, AChar **args)
{
    UnexpectedErrorHandler::getHandler()->registerFilter();

    ModuleManager *moduleManager = ModuleManager::get();
    moduleManager->loadModule(TCHAR("ProgramCore"));
    initializeCmdArguments();

    if (!ProgramCmdLine::get().parse(args, argsc))
    {
        // We cannot initialize logger before parsing command line args
        Logger::initialize();
        LOG_ERROR("CPPReflect", "Failed to parse command line arguments");
        ProgramCmdLine::get().printCommandLine();
    }
    Logger::initialize();
    if (!ProgramCmdLine::get().hasArg(ReflectToolCmdLineConst::LOG_VERBOSE))
    {
        Logger::pushMuteSeverities(Logger::Verbose | Logger::Debug | Logger::Log);
    }
    if (ProgramCmdLine::get().printHelp())
    {
        // Since this invocation is for printing help
        return 0;
    }

    // Loading other libraries
    moduleManager->loadModule(TCHAR("ReflectionRuntime"));
    moduleManager->getOrLoadLibrary(PathFunctions::combinePath(
        TCHAR(LLVM_INSTALL_PATH), TCHAR("bin"), String(LIB_PREFIX) + TCHAR("libclang.") + SHARED_LIB_EXTENSION).getChar()
    );

    Logger::flushStream();

    int32 returnCode = 0;
    if (ProgramCmdLine::get().hasArg(ReflectToolCmdLineConst::SAMPLE_CODE))
    {
        LOG_DEBUG("CPPReflect", "Executing sample codes");
        String srcDir = ProgramCmdLine::get().atIdx(ProgramCmdLine::get().cmdLineCount() - 1);

        SampleCode::testLibClangParsing(srcDir);
        // SampleCode::testTypesAndProperties();
        SampleCode::testPropertySystem();
        // SampleCode::testRegex();
        // SampleCode::testTemplateReflectionGeneration();
    }
    else
    {
        CBE_START_PROFILER();
        copat::JobSystem js(copat::JobSystem::NoSpecialThreads | THREADCONSTRAINT_ENUM_TO_FLAGBIT(NoWorkerAffinity));
        js.initialize({}, nullptr);

        StopWatch sw;
        ModuleSources moduleSrcs;
        bool bGenerated = false;
        if (!moduleSrcs.compileAllSources(SourceGenerator::issueFullRecompile()))
        {
            LOG_ERROR("ModuleReflectTool", "Compiling module sources failed");
            bGenerated = false;
            returnCode = 1;
        }
        else
        {
            SourceGenerator generator;
            generator.initialize(&moduleSrcs);
            generator.parseSources();
            generator.writeGeneratedFiles();

            std::vector<const SourceInformation *> generatedSrcs;
            std::vector<ReflectedTypeItem> allKnownReflectedTypes;
            bGenerated = generator.generatedSources(generatedSrcs);
            allKnownReflectedTypes.insert(
                allKnownReflectedTypes.begin(), generator.getKnownReflectedTypes().cbegin(), generator.getKnownReflectedTypes().cend()
            );

            moduleSrcs.injectGeneratedFiles(generatedSrcs, std::move(allKnownReflectedTypes));
        }

        if (!bGenerated)
        {
            LOG_ERROR("ModuleReflectTool", "Generating module sources failed");
            returnCode = 1;
        }
        else
        {
            SCOPED_MUTE_LOG_SEVERITIES(Logger::Debug);
            String moduleName;
            ProgramCmdLine::get().getArg(moduleName, ReflectToolCmdLineConst::MODULE_NAME);
            sw.stop();
            LOG("ModuleReflectTool", "{} : Reflected in {:0.2} seconds", moduleName, sw.duration());
        }

        js.shutdown();
        CBE_STOP_PROFILER();
    }

    moduleManager->unloadModule(TCHAR("ReflectionRuntime"));
    moduleManager->unloadModule(TCHAR("ProgramCore"));

    UnexpectedErrorHandler::getHandler()->unregisterFilter();
    Logger::shutdown();
    return returnCode;
}

#include "Types/CoreTypes.h"
#include "Logger/Logger.h"
#include "Modules/ModuleManager.h"
#include "Types/Platform/LFS/PlatformLFS.h"
#include "SampleCode.h"
#include <array>
#include "Types/TypesInfo.h"

int32 main(int32 argsc, AChar** args)
{
    UnexpectedErrorHandler::getHandler()->registerFilter();

    ModuleManager* moduleManager = ModuleManager::get();
    moduleManager->loadModule("ProgramCore");
    moduleManager->loadModule("ReflectionRuntime");
    moduleManager->getOrLoadLibrary(FileSystemFunctions::combinePath(LLVM_INSTALL_PATH, "bin", COMBINE(LIB_PREFIX, COMBINE("libclang.", SHARED_LIB_EXTENSION))));
    Logger::log("CPPReflect", "%s(): Reflecting, Engine modules path %s", __func__, ENGINE_MODULES_PATH);
    Logger::log("CPPReflect", "CPP Reflection main\n Args : ");
    String srcDir;
    for (int32 i = 0; i < argsc; ++i)
    {
        Logger::log("CPPReflect", "\t%s", args[i]);
        srcDir = args[i];
    }

    //SampleCode::testLibClangParsing(srcDir);
    SampleCode::testTypesAndProperties();
    SampleCode::testPropertySystem();
    //SampleCode::testRegex();

    moduleManager->unloadModule("ReflectionRuntime");
    moduleManager->unloadModule("ProgramCore");

    UnexpectedErrorHandler::getHandler()->unregisterFilter();
    Logger::flushStream();
    return 0;
}
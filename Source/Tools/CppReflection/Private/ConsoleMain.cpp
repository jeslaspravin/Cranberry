
#include "Types/CoreTypes.h"
#include "Logger/Logger.h"
#include "Modules/ModuleManager.h"
#include "Types/Platform/LFS/PlatformLFS.h"
#include "Types/Platform/PlatformFunctions.h"
#include "TestCode.h"


int32 main(int32 argsc, AChar** args)
{
    ModuleManager::get()->loadModule("ProgramCore");
    ModuleManager::get()->getOrLoadLibrary(FileSystemFunctions::combinePath(LLVM_INSTALL_PATH, "bin", COMBINE(LIB_PREFIX, COMBINE("libclang.", SHARED_LIB_EXTENSION))));
    Logger::log("CPPReflect", "%s(): Reflecting, Engine modules path %s", __func__, ENGINE_MODULES_PATH);
    Logger::log("CPPReflect", "CPP Reflection main\n Args : ");
    String srcDir;
    for (int32 i = 0; i < argsc; ++i)
    {
        Logger::log("CPPReflect", "\t%s", args[i]);
        srcDir = args[i];
    }

    void* procHandle = PlatformFunctions::getCurrentProcessHandle();
    uint32 modulesSize;
    PlatformFunctions::getAllModules(procHandle, nullptr, modulesSize);
    std::vector<LibPointerPtr> libptrs(modulesSize);
    PlatformFunctions::getAllModules(procHandle, libptrs.data(), modulesSize);
    libptrs.resize(modulesSize);

    TestCode::testCode(srcDir);

    ModuleManager::get()->unloadModule("ProgramCore");
    Logger::flushStream();
    return 0;
}
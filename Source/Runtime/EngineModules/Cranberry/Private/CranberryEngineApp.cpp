/*!
 * \file CranberryEngineApp.cpp
 *
 * \author Jeslas
 * \date July 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "CranberryEngineApp.h"
#include "CBEPackage.h"
#include "CoreObjectGC.h"
#include "Classes/EngineBase.h"
#include "CBEObjectHelpers.h"
#include "Modules/ModuleManager.h"
#include "String/StringID.h"
#include "AssetImporter.h"

#include "Classes/WorldsManager.h"

void tempTest();
void tempTickTest();

void CranberryEngineApp::onStart()
{
    rttiModule = IReflectionRuntimeModule::get();
    coreObjModule = ICoreObjectsModule::get();
    bool bModulesLoaded = ModuleManager::get()->loadModule(TCHAR("EngineCore"));
    CBEClass engineClass = nullptr;
#if EDITOR_BUILD
    ModuleManager::get()->addAdditionalLibPath(TCHAR("Editor"));
    bModulesLoaded
        = bModulesLoaded && ModuleManager::get()->loadModule(TCHAR("EditorCore")) && ModuleManager::get()->loadModule(TCHAR("CBEEditor"));

    engineClass = rttiModule->getClassType(STRID("cbe::EditorEngine"));
#else  // EDITOR_BUILD
    engineClass = rttiModule->getClassType(STRID("cbe::CBEGameEngine"));
#endif // EDITOR_BUILD
    fatalAssertf(bModulesLoaded, "Failed loading modules!");
    fatalAssertf(engineClass, "Engine class not found!");
    // This will create and assigns GCBEEngine
    cbe::create(engineClass, TCHAR("CBEEngine"), coreObjModule->getTransientPackage(), cbe::EObjectFlagBits::ObjFlag_RootObject);
    fatalAssertf(GCBEEngine, "Engine %s creation failed", engineClass->nameString);

    GCBEEngine->onStart();

    tempTest();
}

void CranberryEngineApp::onTick()
{
    GCBEEngine->onTick();
    // 8ms, Reduce if this is too much
    coreObjModule->getGC().collect(0.008f);
}

void CranberryEngineApp::onExit()
{
    GCBEEngine->onExit();
    GCBEEngine->worldManager()->unloadAllWorlds();
    // Wait until all the dereferenced objects are cleared, Give as much time as it wants
    coreObjModule->getGC().collect(0.f);
    while (!coreObjModule->getGC().isGcComplete() || coreObjModule->getGC().getLastClearCount() > 0)
    {
        coreObjModule->getGC().collect(0.f);
    }

    ModuleManager *moduleManager = ModuleManager::get();

#if EDITOR_BUILD
    moduleManager->releaseModule(TCHAR("CBEEditor"));
    moduleManager->releaseModule(TCHAR("EditorCore"));
#endif // EDITOR_BUILD
}

void CranberryEngineApp::onRendererStateEvent(ERenderStateEvent state) {}

#include "Types/Platform/LFS/Paths.h"
#include "Property/PropertyHelper.h"
#include "CoreObjectDelegates.h"
#include "ObjectTemplate.h"
#include "IApplicationModule.h"
#include "Types/Platform/LFS/PathFunctions.h"
#include "IEditorCore.h"
#include "Classes/World.h"

void tempTest()
{
    String dir = Paths::contentDirectory();
    String name = Paths::applicationName();
    CoreObjectDelegates::broadcastContentDirectoryAdded(dir);
#if 0
    if (BasicPackagedObject *obj = cbe::load<BasicPackagedObject>(name))
    {
        LOG("Test", "Loaded object %s nameVal %s", obj->getFullPath(), obj->nameVal);
    }
    else
    {
        cbe::Package *package = cbe::Package::createPackage(name, dir);
        cbe::Package *package2 = cbe::Package::createPackage(name + TCHAR("2"), dir);

        BasicPackagedObject *packedObj2 = cbe::create<BasicPackagedObject>(name, package2);
        packedObj2->dt = 0.56;
        packedObj2->nameVal = TCHAR("Its connected object");
        packedObj2->structData = { .a = 4124111.06, .b = 2026, .testStr = "This must be connected to another package" };
        BasicPackagedObject *packedObj = cbe::create<BasicPackagedObject>(name, package);
        packedObj->dt = 0.28;
        packedObj->id = STRID("Hello Subity & Jeslas");
        packedObj->nameVal = TCHAR("Its Me Jeslas");
        packedObj->idxToStr = {
            {1, TCHAR("Jeslas Pravin")},
            {2, TCHAR("Subity Jerald")}
        };
        packedObj->structData = { .a = 8235.28, .b = 834435, .testStr = "3528" };
        packedObj->interLinked = packedObj2;

        BasicFieldSerializedObject *testTemp = cbe::create<BasicFieldSerializedObject>(name, package);
        testTemp->dt = 101.111;
        testTemp->id = STRID("HEll Let lOsE");
        testTemp->interLinked = packedObj;
        testTemp->nameVal = TCHAR("Test All field serialization!");
        testTemp->structData = { .a = 4321, .b = 1234, .testStr = "Not a default value here!" };
        testTemp->idxToStr[10] = {
            {TCHAR("ABC"), 123},
            {TCHAR("CBA"), 321}
        };
        testTemp->idxToStr[5] = {
            {TCHAR("XYZ"), 55667788},
            {TCHAR("ZYX"),     8235}
        };
        IInterfaceExample *interface1 = cbe::cast<IInterfaceExample>(static_cast<cbe::Object *>(testTemp));
        IInterfaceExample2 *interface2 = cbe::cast<IInterfaceExample2>(static_cast<cbe::Object *>(testTemp));
        IInterfaceExample2 *interface3 = cbe::cast<IInterfaceExample2>(interface1);
        BasicFieldSerializedObject *ixToClassObj = cbe::cast<BasicFieldSerializedObject>(interface1);
        cbe::Object *ix1ToClassObj = cbe::cast<cbe::Object>(interface1);
        cbe::Object *ix2ToClassObj = cbe::cast<cbe::Object>(interface2);
        BasicPackagedObject *failingCast = cbe::cast<BasicPackagedObject>(interface1);

        BasicFieldSerializedObject *copied = cbe::cast<BasicFieldSerializedObject>(cbe::duplicateObject(testTemp, package));

        cbe::save(packedObj);
        cbe::save(packedObj2);
    }
#endif

    // const TChar *meshPath = TCHAR("D:/Workspace/Blender/Exports/Sponza.obj");
    // const TChar *meshPath = TCHAR("D:/Workspace/VisualStudio/Cranberry/External/Assets/Cone.obj");
    const TChar *meshPath = TCHAR("D:/Workspace/Blender/Exports/TestScene.obj");
    ImportOption opt;
    opt.filePath = meshPath;
    opt.importContentPath = PathFunctions::combinePath(Paths::engineRuntimeRoot(), TCHAR("Content"));
    if (AssetImporterBase *importer = IEditorCore::get()->findAssetImporter(opt))
    {
        bool bImportAsScene = true;
        static_cast<const MemberFieldWrapper *>(PropertyHelper::findField(opt.structType, STRID("bImportAsScene"))->fieldPtr)
                                                    ->setTypeless(&bImportAsScene, opt.optionsStruct);

        std::vector<cbe::Object *> objs = importer->tryImporting(opt);
        for (cbe::Object *obj : objs)
        {
            cbe::save(obj);
        }
        GCBEEngine->worldManager()->initWorld(cast<cbe::World>(objs[0]), true);
    }
}

void tempTickTest() {}
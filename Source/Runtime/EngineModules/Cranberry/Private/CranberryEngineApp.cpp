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
    // This will create and assigns gCBEEngine
    cbe::create(engineClass, TCHAR("CBEEngine"), coreObjModule->getTransientPackage(), cbe::EObjectFlagBits::ObjFlag_RootObject);
    fatalAssertf(gCBEEngine, "Engine %s creation failed", engineClass->nameString);

    gCBEEngine->onStart();

    tempTest();
}

void CranberryEngineApp::onTick()
{
    gCBEEngine->onTick();
    // 4ms, Reduce if this is too much
    coreObjModule->getGC().collect(0.004f);
}

void CranberryEngineApp::onExit()
{
    gCBEEngine->onExit();
    gCBEEngine->worldManager()->unloadAllWorlds();

    const bool bWaitClearGC = false;
    if (bWaitClearGC)
    { // Wait until all the dereferenced objects are cleared, Give as much time as it wants
        coreObjModule->getGC().collect(0.f);
        while (!coreObjModule->getGC().isGcComplete() || coreObjModule->getGC().getLastClearCount() > 0)
        {
            coreObjModule->getGC().collect(0.f);
        }
    }
    else
    {
        coreObjModule->getGC().purgeAll();
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
#include "Classes/StaticMesh.h"
#include "Classes/ActorPrefab.h"
#include "Classes/Actor.h"
#include "Components/StaticMeshComponent.h"
#include "RenderApi/RenderTaskHelpers.h"
#include "EditorHelpers.h"

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

    bool bUseCubeScene = true;
    if (bUseCubeScene)
    {
        const TChar *scenePath = TCHAR("Scenes/TestCubes:TestCubes");
        cbe::World *sceneObj = cbe::getOrLoad<cbe::World>(scenePath);
        if (sceneObj == nullptr)
        {
            String importContentTo = PathFunctions::combinePath(Paths::engineRuntimeRoot(), TCHAR("Content"));
            cbe::StaticMesh *cubeMesh = cbe::getOrLoad<cbe::StaticMesh>(TCHAR("Meshes/Cube:Cube"));
            if (!cbe::isValid(cubeMesh))
            {
                const TChar *cubeObjPath = TCHAR("D:/Assets/EngineAssets/Cube.obj");
                ImportOption opt;
                opt.filePath = cubeObjPath;
                opt.importContentPath = importContentTo;
                opt.relativeDirPath = TCHAR("Meshes");
                if (AssetImporterBase *importer = IEditorCore::get()->findAssetImporter(opt))
                {
                    bool bImportAsScene = false;
                    static_cast<const MemberFieldWrapper *>(PropertyHelper::findField(opt.structType, STRID("bImportAsScene"))->fieldPtr)
                                                                ->setTypeless(&bImportAsScene, opt.optionsStruct);

                    std::vector<cbe::Object *> objs = importer->tryImporting(opt);
                    debugAssert(objs.size() == 1);

                    cubeMesh = cbe::cast<cbe::StaticMesh>(objs[0]);
                    cbe::save(cubeMesh);
                }
            }

            debugAssert(cubeMesh);

            cbe::Package *worldPackage = cbe::Package::createPackage(TCHAR("Scenes/TestCubes"), importContentTo, false);
            debugAssert(worldPackage);
            cbe::markDirty(worldPackage);
            SET_BITS(cbe::INTERNAL_ObjectCoreAccessors::getFlags(worldPackage), cbe::EObjectFlagBits::ObjFlag_PackageLoaded);

            sceneObj = cbe::create<cbe::World>(TCHAR("TestCubes"), worldPackage, cbe::EObjectFlagBits::ObjFlag_PackageLoaded);

            // Create hundred thousand sm objects
            for (uint32 i = 0; i < 10 * 100 * 100; ++i)
            {
                cbe::ActorPrefab *smActorPrefab = cbe::ActorPrefab::prefabFromActorTemplate(cbe::ActorPrefab::objectTemplateFromObj(
                    EditorHelpers::addActorToWorld(sceneObj, cbe::Actor::staticType(), TCHAR("CubeActor_") + String::toString(i), 0)
                ));
                cbe::Actor *smActor = smActorPrefab->getActorTemplate();
                cbe::StaticMeshComponent *smComp = static_cast<cbe::StaticMeshComponent *>(
                    EditorHelpers::addComponentToPrefab(smActorPrefab, cbe::StaticMeshComponent::staticType(), TCHAR("CubeSM"))
                );

                cbe::Object *modifyingComp = EditorHelpers::modifyPrefabCompField(
                    PropertyHelper::findField(smComp->getType(), GET_MEMBER_ID_CHECKED(cbe::StaticMeshComponent, mesh)), smComp
                );
                modifyingComp = EditorHelpers::modifyPrefabCompField(PropertyHelper::findField(smComp->getType(), STRID("relativeTf")), smComp);
                debugAssert(modifyingComp == smComp);
                smComp->mesh = cubeMesh;

                Vector3D pos;
                pos.x() = (i % 100) * 50.0f + 25 - 2500;
                pos.y() = ((i / 100) % 100) * 50.0f + 25 - 2500;
                pos.z() = (i / (100 * 100)) * 50.0f + 25 - 250;
                smComp->setRelativeLocation(pos);
                smComp->setRelativeScale(Vector3D{ 0.25f });

                // Reset root and remove default root component
                cbe::TransformComponent *defaultRoot = smActorPrefab->getRootComponent();
                smActorPrefab->setRootComponent(smComp);
                smActorPrefab->removeComponent(defaultRoot);
            }

            cbe::save(sceneObj);
        }
        if (sceneObj)
        {
            gCBEEngine->worldManager()->initWorld(sceneObj, true);
        }
    }
    else
    {

        const TChar *meshObjPath = TCHAR("D:/Workspace/Blender/Exports/Sponza.obj");
        const TChar *meshEnginePath = TCHAR("Scenes/Sponze:Sponza");
        cbe::World *sceneObj = cbe::getOrLoad<cbe::World>(meshEnginePath);
        if (sceneObj == nullptr)
        {
            ImportOption opt;
            opt.filePath = meshObjPath;
            opt.importContentPath = PathFunctions::combinePath(Paths::engineRuntimeRoot(), TCHAR("Content"));
            opt.relativeDirPath = TCHAR("Scenes");
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
                if (!objs.empty())
                {
                    sceneObj = cbe::cast<cbe::World>(objs[0]);
                }
            }
        }

        if (sceneObj)
        {
            gCBEEngine->worldManager()->initWorld(sceneObj, true);
        }
    }
}

void tempTickTest() {}
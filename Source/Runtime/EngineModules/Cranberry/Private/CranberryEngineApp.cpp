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

void tempTest();

void CranberryEngineApp::onStart() { tempTest(); }

void CranberryEngineApp::onTick() {}

void CranberryEngineApp::onExit() {}

void CranberryEngineApp::onRendererStateEvent(ERenderStateEvent state) {}

#include "CBEPackage.h"
#include "CBEObjectHelpers.h"
#include "Types/Platform/LFS/Paths.h"
#include "Property/PropertyHelper.h"
#include "CoreObjectDelegates.h"
#include "ObjectTemplate.h"
#include "Classes/GameEngine.h"
#include "Modules/ModuleManager.h"

void tempTest()
{
    String dir = Paths::contentDirectory();
    String name = Paths::applicationName();
    CoreObjectDelegates::broadcastContentDirectoryAdded(dir);
#if 0
    if (BasicPackagedObject *obj = CBE::load<BasicPackagedObject>(name))
    {
        LOG("Test", "Loaded object %s nameVal %s", obj->getFullPath(), obj->nameVal);
    }
    else
    {
        CBE::Package *package = CBE::Package::createPackage(name, dir);
        CBE::Package *package2 = CBE::Package::createPackage(name + TCHAR("2"), dir);

        BasicPackagedObject *packedObj2 = CBE::create<BasicPackagedObject>(name, package2);
        packedObj2->dt = 0.56;
        packedObj2->nameVal = TCHAR("Its connected object");
        packedObj2->structData = { .a = 4124111.06, .b = 2026, .testStr = "This must be connected to another package" };
        BasicPackagedObject *packedObj = CBE::create<BasicPackagedObject>(name, package);
        packedObj->dt = 0.28;
        packedObj->id = STRID("Hello Subity & Jeslas");
        packedObj->nameVal = TCHAR("Its Me Jeslas");
        packedObj->idxToStr = {
            {1, TCHAR("Jeslas Pravin")},
            {2, TCHAR("Subity Jerald")}
        };
        packedObj->structData = { .a = 8235.28, .b = 834435, .testStr = "3528" };
        packedObj->interLinked = packedObj2;

        BasicFieldSerializedObject *testTemp = CBE::create<BasicFieldSerializedObject>(name, package);
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
        IInterfaceExample *interface1 = CBE::cast<IInterfaceExample>(static_cast<CBE::Object *>(testTemp));
        IInterfaceExample2 *interface2 = CBE::cast<IInterfaceExample2>(static_cast<CBE::Object *>(testTemp));
        IInterfaceExample2 *interface3 = CBE::cast<IInterfaceExample2>(interface1);
        BasicFieldSerializedObject *ixToClassObj = CBE::cast<BasicFieldSerializedObject>(interface1);
        CBE::Object *ix1ToClassObj = CBE::cast<CBE::Object>(interface1);
        CBE::Object *ix2ToClassObj = CBE::cast<CBE::Object>(interface2);
        BasicPackagedObject *failingCast = CBE::cast<BasicPackagedObject>(interface1);

        BasicFieldSerializedObject *copied = CBE::cast<BasicFieldSerializedObject>(CBE::duplicateObject(testTemp, package));

        CBE::save(packedObj);
        CBE::save(packedObj2);
    }
#endif
    if (CBE::ObjectTemplate *obj = CBE::load<CBE::ObjectTemplate>(name))
    {
        CBE::GameEngine *engine = CBE::cast<CBE::GameEngine>(CBE::create(obj, TCHAR("Engine"), nullptr));
        LOG("Test", "Loaded object %s nameVal %s", obj->getFullPath(), engine->nameVal);
    }
    else
    {
        CBE::Package *package = CBE::Package::createPackage(name + TCHAR("_Template"), dir);

        CBE::ObjectTemplate *tmplt
            = CBE::create<CBE::ObjectTemplate, StringID, String>(name, package, 0, CBE::GameEngine::staticType()->name, name);
        CBE::cast<CBE::GameEngine>(tmplt->getTemplate())->nameVal = TCHAR("Test value, I am Jeslas Pravin");
        tmplt->onFieldModified(PropertyHelper::findField(CBE::GameEngine::staticType(), STRID("nameVal")), tmplt->getTemplate());

        CBE::save(package);
    }

    // TODO
    ModuleManager::get()->addAdditionalLibPath(TCHAR("Editor"));
    ModuleManager::get()->loadModule(TCHAR("EditorCore"));
}
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
#include "Widgets/ImGui/IImGuiLayer.h"
#include "IApplicationModule.h"
#include "Widgets/ImGui/ImGuiManager.h"
#include "Widgets/WidgetWindow.h"
#include "Widgets/ImGui/WgImGui.h"

class TestImGuiLayer : public IImGuiLayer
{
public:
    int32 layerDepth() const override { return 0; }

    int32 sublayerDepth() const override { return 0; }

    void draw(ImGuiDrawInterface *drawInterface) override
    {
        bool bOpen = true;
        ImGui::ShowDemoWindow(&bOpen);
    }
};

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
    if (cbe::ObjectTemplate *obj = cbe::load<cbe::ObjectTemplate>(name))
    {
        cbe::GameEngine *engine = cbe::cast<cbe::GameEngine>(cbe::create(obj, TCHAR("Engine"), nullptr));
        LOG("Test", "Loaded object %s nameVal %s", obj->getFullPath(), engine->nameVal);
    }
    else
    {
        cbe::Package *package = cbe::Package::createPackage(name + TCHAR("_Template"), dir);

        cbe::ObjectTemplate *tmplt
            = cbe::create<cbe::ObjectTemplate, StringID, String>(name, package, 0, cbe::GameEngine::staticType()->name, name);
        cbe::cast<cbe::GameEngine>(tmplt->getTemplate())->nameVal = TCHAR("Test value, I am Jeslas Pravin");
        tmplt->onFieldModified(PropertyHelper::findField(cbe::GameEngine::staticType(), STRID("nameVal")), tmplt->getTemplate());

        cbe::save(package);
    }

    // TODO
    ModuleManager::get()->addAdditionalLibPath(TCHAR("Editor"));
    ModuleManager::get()->loadModule(TCHAR("EditorCore"));

    WgImGui::WgArguments args;
    args.imguiManagerName = TCHAR("TestImgui");
    SharedPtr<WgImGui> wgImgui = std::make_shared<WgImGui>();
    wgImgui->construct(args);
    SharedPtr<IImGuiLayer> imguiLayer = std::make_shared<TestImGuiLayer>();
    wgImgui->getImGuiManager().addLayer(imguiLayer);
    IApplicationModule::get()->getApplication()->getMainWindow()->setContent(std::static_pointer_cast<WidgetBase>(wgImgui));
}
/*!
 * \file VulkanRHIModule.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "VulkanRHIModule.h"
#include "Modules/ModuleManager.h"
#include "VulkanGraphicsHelper.h"
#include "VulkanGraphicsInstance.h"

class VulkanRHIModule final : public IVulkanRHIModule
{
private:
    IGraphicsInstance* graphicsInstance = nullptr;
public:
    /* IRHIModule overrides */
    IGraphicsInstance* createGraphicsInstance() final;
    const GraphicsHelperAPI* getGraphicsHelper() const final;

    /* IModuleBase overrides */
    void init() final;
    void release() final;
    void destroyGraphicsInstance() final;
    /* IVulkanRHIModule overrides */
    IGraphicsInstance* getGraphicsInstance() const final;
    /* End overrides */
};

DECLARE_MODULE(VulkanRHI, VulkanRHIModule)

IGraphicsInstance* VulkanRHIModule::createGraphicsInstance()
{
    if (graphicsInstance == nullptr)
    {
        graphicsInstance = new VulkanGraphicsInstance();
    }
    return graphicsInstance;
}

void VulkanRHIModule::destroyGraphicsInstance()
{
    if (graphicsInstance != nullptr)
    {
        delete graphicsInstance;
        graphicsInstance = nullptr;
    }
}

const GraphicsHelperAPI* VulkanRHIModule::getGraphicsHelper() const
{
    static VulkanGraphicsHelper graphicsHelper;
    return &graphicsHelper;
}

void VulkanRHIModule::init()
{

}

void VulkanRHIModule::release()
{
    destroyGraphicsInstance();
}

IGraphicsInstance* VulkanRHIModule::getGraphicsInstance() const
{
    return graphicsInstance;
}

IVulkanRHIModule* IVulkanRHIModule::get()
{
    static WeakModulePtr weakRiModule = (ModuleManager::get()->getOrLoadModule("VulkanRHI"));
    return weakRiModule.expired() ? nullptr : static_cast<IVulkanRHIModule*>(weakRiModule.lock().get());
}

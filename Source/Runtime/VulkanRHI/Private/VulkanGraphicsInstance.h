/*!
 * \file VulkanGraphicsInstance.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once
#include "Memory/SmartPointers.h"
#include "RenderInterface/GraphicsIntance.h"
#include "Types/Platform/GenericPlatformTypes.h"
#include "VulkanInternals/VulkanDevice.h"

#include <vector>
#include <vulkan/vulkan_core.h>

class VulkanGraphicsInstance final : public IGraphicsInstance
{
    friend class VulkanGraphicsHelper;

private:
    std::vector<VkExtensionProperties> availableInstanceExtensions;
    std::vector<const char *> registeredInstanceExtensions;

    VkInstance vulkanInstance;
    VulkanDevice selectedDevice;
    SharedPtr<class IVulkanMemoryAllocator> memoryAllocator;
    SharedPtr<class VulkanDescriptorsSetAllocator> descriptorsSetAllocator;
    SharedPtr<class IRenderCommandList> vulkanCmdList;

    void loadGlobalFunctions();
    void loadInstanceFunctions();

    void createVulkanInstance();
    // Window canvas if provided will be used to check surface capabilities, If not that will be ignored
    void createVulkanDevice(const WindowCanvasRef &windowCanvas);

#if DEV_BUILD
    void collectInstanceLayers(std::vector<const char *> &layers) const;
#endif
    NODISCARD bool collectInstanceExtensions(std::vector<const char *> &extensions) const;

public:
    /* IGraphicsInstance override */

    void load() override;
    void unload() override;
    void updateSurfaceDependents() override;
    void initializeCmds(class IRenderCommandList *commandList) override;

    /* Override ends */
};

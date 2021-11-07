#pragma once
#include "RenderInterface/GraphicsIntance.h"
#include "Types/Platform/GenericPlatformTypes.h"
#include "VulkanRI/VulkanInternals/VulkanDevice.h"
#include "Memory/SmartPointers.h"

#include <vector>
#include "vulkan_core.h"

class ENGINERENDERER_EXPORT VulkanGraphicsInstance final:
    public IGraphicsInstance
{
    friend class VulkanGraphicsHelper;

private:
    
    std::vector<VkExtensionProperties> availableInstanceExtensions;
    std::vector<const char*> registeredInstanceExtensions;

    VkInstance vulkanInstance;
    VulkanDevice selectedDevice;
    SharedPtr<class IVulkanMemoryAllocator> memoryAllocator;
    SharedPtr<class VulkanDescriptorsSetAllocator> descriptorsSetAllocator;
    SharedPtr<class IRenderCommandList> vulkanCmdList;

    void loadGlobalFunctions();
    void loadInstanceFunctions();

    void createVulkanInstance();
    void createVulkanDevice();

#if _DEBUG
    void collectInstanceLayers(std::vector<const char*>& layers) const;
#endif
    [[nodiscard]] bool collectInstanceExtensions(std::vector<const char*>& extensions) const;
public:

    /* IGraphicsInstance override */

    void load() override;
    void unload() override;
    void updateSurfaceDependents() override;
    void initializeCmds(class IRenderCommandList* commandList) override;

    /* Override ends */
};


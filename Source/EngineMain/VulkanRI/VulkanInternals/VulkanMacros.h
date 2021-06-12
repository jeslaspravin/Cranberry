#pragma once

#define VULKAN_KHR_TYPE(AppendTo) AppendTo##KHR
#define VULKAN_KHR_DEF(AppendTo) AppendTo##_KHR

#ifndef CREATE_APP_INFO
#define CREATE_APP_INFO(VariableName)                       \
VkApplicationInfo VariableName;                             \
VariableName.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;    \
VariableName.pNext = nullptr
#endif

#ifndef CREATE_INSTANCE_INFO
#define CREATE_INSTANCE_INFO(VariableName)                      \
VkInstanceCreateInfo VariableName;                              \
VariableName.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;    \
VariableName.pNext = nullptr;                                   \
VariableName.flags = 0;                                         \
VariableName.ppEnabledLayerNames = nullptr;                     \
VariableName.enabledLayerCount = 0
#endif

#ifndef CREATE_DEVICE_INFO
#define CREATE_DEVICE_INFO(VariableName)                    \
VkDeviceCreateInfo VariableName;                            \
VariableName.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;  \
VariableName.pNext = nullptr;                               \
VariableName.flags = 0;                                     \
VariableName.enabledLayerCount = 0;                         \
VariableName.ppEnabledLayerNames = nullptr                        
#endif

#ifndef CREATE_QUEUE_INFO
#define CREATE_QUEUE_INFO(VariableName)                         \
VkDeviceQueueCreateInfo VariableName;                           \
VariableName.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;\
VariableName.pNext = nullptr;                                   \
VariableName.flags = 0
#endif

#ifndef SUBMIT_INFO
#define SUBMIT_INFO(VariableName)                   \
VkSubmitInfo VariableName;                          \
VariableName.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO; \
VariableName.pNext = nullptr;                       \
VariableName.pSignalSemaphores = nullptr;           \
VariableName.pWaitDstStageMask = nullptr;             \
VariableName.pWaitSemaphores = nullptr;             \
VariableName.signalSemaphoreCount = 0;              \
VariableName.waitSemaphoreCount = 0
#endif

#ifndef CREATE_SWAPCHAIN_INFO
#define CREATE_SWAPCHAIN_INFO(VariableName)                         \
VkSwapchainCreateInfoKHR VariableName;                              \
VariableName.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;   \
VariableName.pNext = nullptr;                                       \
VariableName.flags = 0
#endif

#ifndef IMAGE_MEMORY_BARRIER
#define IMAGE_MEMORY_BARRIER(VariableName)                      \
VkImageMemoryBarrier VariableName;                              \
VariableName.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;    \
VariableName.pNext = nullptr;                                   \
VariableName.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED
#endif

#ifndef CREATE_DEBUG_UTILS_MESSENGER_INFO
#define CREATE_DEBUG_UTILS_MESSENGER_INFO(VariableName)                         \
VkDebugUtilsMessengerCreateInfoEXT VariableName;                                \
VariableName.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;   \
VariableName.pNext = nullptr;                                                   \
VariableName.flags = 0;                                                         \
VariableName.pUserData = nullptr
#endif

#ifndef DEBUG_UTILS_OBJECT_NAME_INFO
#define DEBUG_UTILS_OBJECT_NAME_INFO(VariableName)                          \
VkDebugUtilsObjectNameInfoEXT VariableName;                                 \
VariableName.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;    \
VariableName.pNext = nullptr
#endif

#ifndef DEBUG_UTILS_LABEL
#define DEBUG_UTILS_LABEL(VariableName)                         \
VkDebugUtilsLabelEXT VariableName;                              \
VariableName.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;   \
VariableName.pNext = nullptr
#endif

#ifndef PHYSICAL_DEVICE_FEATURES_2
#define PHYSICAL_DEVICE_FEATURES_2(VariableName)                        \
VkPhysicalDeviceFeatures2KHR VariableName;                              \
VariableName.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2_KHR;  \
VariableName.pNext = nullptr
#endif

#ifndef PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES
#define PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES(VariableName)                       \
VkPhysicalDeviceTimelineSemaphoreFeaturesKHR VariableName;                              \
VariableName.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES_KHR; \
VariableName.pNext = nullptr
#endif

#ifndef PHYSICAL_DEVICE_PROPERTIES_2
#define PHYSICAL_DEVICE_PROPERTIES_2(VariableName)                          \
VkPhysicalDeviceProperties2KHR VariableName;                                \
VariableName.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2_KHR;    \
VariableName.pNext = nullptr
#endif

#ifndef PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_PROPERTIES
#define PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_PROPERTIES(VariableName)                         \
VkPhysicalDeviceTimelineSemaphorePropertiesKHR VariableName;                                \
VariableName.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_PROPERTIES_KHR;   \
VariableName.pNext = nullptr
#endif

#ifndef PHYSICAL_DEVICE_MEMORY_PROPERTIES_2
#define PHYSICAL_DEVICE_MEMORY_PROPERTIES_2(VariableName)                       \
VkPhysicalDeviceMemoryProperties2KHR VariableName;                              \
VariableName.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2_KHR; \
VariableName.pNext = nullptr
#endif

#ifndef PHYSICAL_DEVICE_MEMORY_BUDGET_PROPERTIES
#define PHYSICAL_DEVICE_MEMORY_BUDGET_PROPERTIES(VariableName)                      \
VkPhysicalDeviceMemoryBudgetPropertiesEXT VariableName;                             \
VariableName.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_BUDGET_PROPERTIES_EXT;\
VariableName.pNext = nullptr
#endif

#ifndef BUFFER_CREATE_INFO
#define BUFFER_CREATE_INFO(VariableName)                    \
VkBufferCreateInfo VariableName;                            \
VariableName.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;  \
VariableName.flags = 0;                                     \
VariableName.pNext = nullptr;                               \
VariableName.sharingMode = VK_SHARING_MODE_EXCLUSIVE;       \
VariableName.queueFamilyIndexCount = 0;                     \
VariableName.pQueueFamilyIndices = nullptr
#endif

#ifndef BUFFER_VIEW_CREATE_INFO
#define BUFFER_VIEW_CREATE_INFO(VariableName)                   \
VkBufferViewCreateInfo VariableName;                            \
VariableName.sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO; \
VariableName.pNext = nullptr;                                   \
VariableName.flags = 0;                                         \
VariableName.offset = 0;                                        \
VariableName.range = VK_WHOLE_SIZE                              
#endif

#ifndef IMAGE_CREATE_INFO
#define IMAGE_CREATE_INFO(VariableName)                     \
VkImageCreateInfo VariableName;                             \
VariableName.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;   \
VariableName.pNext = 0;                                     \
VariableName.sharingMode = VK_SHARING_MODE_EXCLUSIVE;       \
VariableName.queueFamilyIndexCount = 0;                     \
VariableName.pQueueFamilyIndices = nullptr
#endif

#ifndef IMAGE_VIEW_CREATE_INFO
#define IMAGE_VIEW_CREATE_INFO(VariableName)                    \
VkImageViewCreateInfo VariableName;                             \
VariableName.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;  \
VariableName.pNext = nullptr;                                   \
VariableName.flags = 0;                                         \
VariableName.components = {                                     \
    VkComponentSwizzle::VK_COMPONENT_SWIZZLE_IDENTITY,          \
    VkComponentSwizzle::VK_COMPONENT_SWIZZLE_IDENTITY,          \
    VkComponentSwizzle::VK_COMPONENT_SWIZZLE_IDENTITY,          \
    VkComponentSwizzle::VK_COMPONENT_SWIZZLE_IDENTITY           \
};
#endif

#ifndef SAMPLER_CREATE_INFO
#define SAMPLER_CREATE_INFO(VariableName)                   \
VkSamplerCreateInfo VariableName;                           \
VariableName.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO; \
VariableName.pNext = nullptr;                               \
VariableName.flags = 0;                                     \
VariableName.unnormalizedCoordinates = VK_FALSE
#endif

#ifndef DESCRIPTOR_SET_LAYOUT_CREATE_INFO
#define DESCRIPTOR_SET_LAYOUT_CREATE_INFO(VariableName)                     \
VkDescriptorSetLayoutCreateInfo VariableName;                               \
VariableName.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;   \
VariableName.pNext = nullptr;                                               \
VariableName.flags = 0
#endif

#ifndef DESCRIPTOR_POOL_CREATE_INFO
#define DESCRIPTOR_POOL_CREATE_INFO(VariableName)                   \
VkDescriptorPoolCreateInfo VariableName;                            \
VariableName.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO; \
VariableName.pNext = nullptr;                                       \
VariableName.flags = 0
#endif

#ifndef DESCRIPTOR_SET_ALLOCATE_INFO
#define DESCRIPTOR_SET_ALLOCATE_INFO(VariableName)                  \
VkDescriptorSetAllocateInfo VariableName;                           \
VariableName.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;\
VariableName.pNext = nullptr
#endif

#ifndef WRITE_RESOURCE_TO_DESCRIPTORS_SET
#define WRITE_RESOURCE_TO_DESCRIPTORS_SET(VariableName)                  \
VkWriteDescriptorSet VariableName;                              \
VariableName.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;    \
VariableName.pNext = nullptr;                                   \
VariableName.dstArrayElement = 0;                               \
VariableName.descriptorCount = 1;                               \
VariableName.pBufferInfo = nullptr;                             \
VariableName.pTexelBufferView = nullptr;                        \
VariableName.pImageInfo = nullptr
#endif

#ifndef COPY_DESCRIPTORS_SET_TO_SET
#define COPY_DESCRIPTORS_SET_TO_SET(VariableName)           \
VkCopyDescriptorSet VariableName;                           \
VariableName.sType = VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET; \
VariableName.pNext = nullptr;                               \
VariableName.descriptorCount = 1;                           \
VariableName.srcArrayElement = 0;                           \
VariableName.dstArrayElement = 0
#endif

#ifndef RENDERPASS_CREATE_INFO
#define RENDERPASS_CREATE_INFO(VariableName)                    \
VkRenderPassCreateInfo VariableName;                            \
VariableName.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO; \
VariableName.flags = 0;                                         \
VariableName.pNext = nullptr
#endif

#ifndef FRAMEBUFFER_CREATE_INFO
#define FRAMEBUFFER_CREATE_INFO(VariableName)                   \
VkFramebufferCreateInfo VariableName;                           \
VariableName.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO; \
VariableName.flags = 0;                                         \
VariableName.pNext = nullptr
#endif

#ifndef RENDERPASS_BEGIN_INFO
#define RENDERPASS_BEGIN_INFO(VariableName)                     \
VkRenderPassBeginInfo VariableName;                             \
VariableName.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;  \
VariableName.pNext = nullptr
#endif

#ifndef SHADER_MODULE_CREATE_INFO
#define SHADER_MODULE_CREATE_INFO(VariableName)                     \
VkShaderModuleCreateInfo VariableName;                              \
VariableName.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;   \
VariableName.pNext = nullptr;                                       \
VariableName.flags = 0
#endif

#ifndef PIPELINE_CACHE_CREATE_INFO
#define PIPELINE_CACHE_CREATE_INFO(VariableName)                    \
VkPipelineCacheCreateInfo VariableName;                             \
VariableName.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;  \
VariableName.pNext = nullptr;                                       \
VariableName.flags = 0;                                             \
VariableName.initialDataSize = 0;                                   \
VariableName.pInitialData = nullptr
#endif

#ifndef PIPELINE_LAYOUT_CREATE_INFO
#define PIPELINE_LAYOUT_CREATE_INFO(VariableName)                   \
VkPipelineLayoutCreateInfo VariableName;                            \
VariableName.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO; \
VariableName.pNext = nullptr;                                       \
VariableName.flags = 0
#endif

#ifndef GRAPHICS_PIPELINE_CREATE_INFO
#define GRAPHICS_PIPELINE_CREATE_INFO(VariableName)                     \
VkGraphicsPipelineCreateInfo VariableName;                              \
VariableName.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;   \
VariableName.pNext = nullptr;                                           \
VariableName.flags = 0;                                                 \
VariableName.basePipelineIndex = -1;                                    \
VariableName.basePipelineHandle = VK_NULL_HANDLE
#endif

#ifndef PIPELINE_SHADER_STAGE_CREATE_INFO
#define PIPELINE_SHADER_STAGE_CREATE_INFO(VariableName)                     \
VkPipelineShaderStageCreateInfo VariableName;                               \
VariableName.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;   \
VariableName.pNext = nullptr;                                               \
VariableName.flags = 0;                                                     \
VariableName.pSpecializationInfo = VK_NULL_HANDLE
#endif

#ifndef PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO
#define PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO(VariableName)                   \
VkPipelineVertexInputStateCreateInfo VariableName;                              \
VariableName.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO; \
VariableName.pNext = nullptr;                                                   \
VariableName.flags = 0
#endif

#ifndef PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO
#define PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO(VariableName)                     \
VkPipelineInputAssemblyStateCreateInfo VariableName;                                \
VariableName.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;   \
VariableName.pNext = nullptr;                                                       \
VariableName.flags = 0;                                                             \
VariableName.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;/*As it is common*/     \
VariableName.primitiveRestartEnable = VK_FALSE
#endif

#ifndef PIPELINE_TESSELLATION_STATE_CREATE_INFO
#define PIPELINE_TESSELLATION_STATE_CREATE_INFO(VariableName)                   \
VkPipelineTessellationStateCreateInfo VariableName;                             \
VariableName.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO; \
VariableName.pNext = nullptr;                                                   \
VariableName.flags = 0
#endif

#ifndef PIPELINE_VIEWPORT_STATE_CREATE_INFO
#define PIPELINE_VIEWPORT_STATE_CREATE_INFO(VariableName)                   \
VkPipelineViewportStateCreateInfo VariableName;                             \
VariableName.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO; \
VariableName.pNext = nullptr;                                               \
VariableName.flags = 0
#endif

#ifndef PIPELINE_RASTERIZATION_STATE_CREATE_INFO
#define PIPELINE_RASTERIZATION_STATE_CREATE_INFO(VariableName)                  \
VkPipelineRasterizationStateCreateInfo VariableName;                            \
VariableName.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;\
VariableName.pNext = nullptr;                                                   \
VariableName.flags = 0;                                                         \
VariableName.rasterizerDiscardEnable = VK_FALSE;                                \
VariableName.depthClampEnable = VK_FALSE;                                       \
VariableName.lineWidth = 1.0f;                                                  \
VariableName.depthBiasEnable = VK_FALSE;                                        \
VariableName.depthBiasClamp = VariableName.depthBiasConstantFactor = 0
#endif

#ifndef PIPELINE_MULTISAMPLE_STATE_CREATE_INFO
#define PIPELINE_MULTISAMPLE_STATE_CREATE_INFO(VariableName)                    \
VkPipelineMultisampleStateCreateInfo VariableName;                              \
VariableName.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;  \
VariableName.pNext = nullptr;                                                   \
VariableName.flags = 0
#endif

#ifndef PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO
#define PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO(VariableName)                  \
VkPipelineDepthStencilStateCreateInfo VariableName;                             \
VariableName.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;\
VariableName.pNext = nullptr;                                                   \
VariableName.flags = 0
#endif

#ifndef PIPELINE_COLOR_BLEND_STATE_CREATE_INFO
#define PIPELINE_COLOR_BLEND_STATE_CREATE_INFO(VariableName)                    \
VkPipelineColorBlendStateCreateInfo VariableName;                               \
VariableName.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;  \
VariableName.pNext = nullptr;                                                   \
VariableName.flags = 0;                                                         \
VariableName.logicOpEnable = VK_FALSE;                                          \
VariableName.logicOp = VK_LOGIC_OP_NO_OP
#endif

#ifndef PIPELINE_DYNAMIC_STATE_CREATE_INFO
#define PIPELINE_DYNAMIC_STATE_CREATE_INFO(VariableName)                        \
VkPipelineDynamicStateCreateInfo VariableName;                                  \
VariableName.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;      \
VariableName.pNext = nullptr;                                                   \
VariableName.flags = 0
#endif

#ifndef COMPUTE_PIPELINE_CREATE_INFO
#define COMPUTE_PIPELINE_CREATE_INFO(VariableName)                      \
VkComputePipelineCreateInfo VariableName;                               \
VariableName.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;    \
VariableName.pNext = nullptr;                                           \
VariableName.flags = 0;                                                 \
VariableName.basePipelineIndex = -1;                                    \
VariableName.basePipelineHandle = VK_NULL_HANDLE
#endif

#ifndef CREATE_SEMAPHORE_INFO
#define CREATE_SEMAPHORE_INFO(VariableName)                     \
VkSemaphoreCreateInfo  VariableName;                            \
VariableName.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;   \
VariableName.pNext = nullptr;                                   \
VariableName.flags = 0
#endif

// TODO(Jeslas)(API Update) : Change and remove this macro once driver providers update to Vulkan 1.2
#if VK_VERSION_1_2
#ifndef TIMELINE_SEMAPHORE_TYPE
#define TIMELINE_SEMAPHORE_TYPE(VarType) VarType
#endif
#ifndef TIMELINE_SEMAPHORE_DEF
#define TIMELINE_SEMAPHORE_DEF(Def) Def 
#endif
#else
#ifndef TIMELINE_SEMAPHORE_TYPE
#define TIMELINE_SEMAPHORE_TYPE(VarType) VULKAN_KHR_TYPE(VarType)
#endif
#ifndef TIMELINE_SEMAPHORE_DEF
#define TIMELINE_SEMAPHORE_DEF(Def) VULKAN_KHR_DEF(Def)
#endif
#endif                      

#ifndef CREATE_TYPED_SEMAPHORE_INFO
#define CREATE_TYPED_SEMAPHORE_INFO(VariableName)                                           \
TIMELINE_SEMAPHORE_TYPE(VkSemaphoreTypeCreateInfo) VariableName;                            \
VariableName.sType = TIMELINE_SEMAPHORE_DEF(VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO); \
VariableName.pNext = nullptr;                                                               \
VariableName.initialValue = 0;                                                              \
VariableName.semaphoreType = TIMELINE_SEMAPHORE_DEF(VK_SEMAPHORE_TYPE_TIMELINE)
#endif

#ifndef SEMAPHORE_SIGNAL_INFO
#define SEMAPHORE_SIGNAL_INFO(VariableName)                                             \
TIMELINE_SEMAPHORE_TYPE(VkSemaphoreSignalInfo) VariableName;                            \
VariableName.sType = TIMELINE_SEMAPHORE_DEF(VK_STRUCTURE_TYPE_SEMAPHORE_SIGNAL_INFO);  \
VariableName.pNext = nullptr
#endif

#ifndef SEMAPHORE_WAIT_INFO
#define SEMAPHORE_WAIT_INFO(VariableName)                                           \
TIMELINE_SEMAPHORE_TYPE(VkSemaphoreWaitInfo) VariableName;                          \
VariableName.sType = TIMELINE_SEMAPHORE_DEF(VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO);\
VariableName.pNext = nullptr;                                                       \
VariableName.flags = TIMELINE_SEMAPHORE_DEF(VK_SEMAPHORE_WAIT_ANY_BIT)
#endif

#ifndef CREATE_FENCE_INFO
#define CREATE_FENCE_INFO(VariableName)                     \
VkFenceCreateInfo VariableName;                             \
VariableName.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;   \
VariableName.flags = 0;                                     \
VariableName.pNext = nullptr
#endif

#ifndef MEMORY_ALLOCATE_INFO
#define MEMORY_ALLOCATE_INFO(VariableName)                      \
VkMemoryAllocateInfo VariableName;                              \
VariableName.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;    \
VariableName.pNext = nullptr
#endif

#ifndef CREATE_COMMAND_POOL_INFO
#define CREATE_COMMAND_POOL_INFO(VariableName)                      \
VkCommandPoolCreateInfo VariableName;                               \
VariableName.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;    \
VariableName.pNext = nullptr
#endif

#ifndef CMD_BUFFER_ALLOC_INFO
#define CMD_BUFFER_ALLOC_INFO(VariableName)                         \
VkCommandBufferAllocateInfo VariableName;                           \
VariableName.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;\
VariableName.pNext = nullptr;                                       \
VariableName.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY
#endif

#ifndef CMD_BUFFER_BEGIN_INFO
#define CMD_BUFFER_BEGIN_INFO(VariableName)                         \
VkCommandBufferBeginInfo VariableName;                              \
VariableName.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;   \
VariableName.pNext = nullptr;                                       \
VariableName.pInheritanceInfo = nullptr
#endif

#ifndef PRESENT_INFO
#define PRESENT_INFO(VariableName)                          \
VkPresentInfoKHR VariableName;                              \
VariableName.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;    \
VariableName.pNext = nullptr
#endif

#ifndef DECLARE_VK_GRAPHICS_RESOURCE
#define DECLARE_VK_GRAPHICS_RESOURCE(NewTypeName,NewTypeTemplates,BaseTypeName,BaseTypeTemplates)\
    DECLARE_GRAPHICS_RESOURCE(NewTypeName,NewTypeTemplates,BaseTypeName,BaseTypeTemplates) \
private: \
    static String OBJ_TYPE_NAME;\
public: \
    virtual VkObjectType getObjectType() const override; \
    virtual const String& getObjectTypeName() const override;
#endif // DECLARE_VK_GRAPHICS_RESOURCE

#ifndef DEFINE_VK_GRAPHICS_RESOURCE
#define DEFINE_VK_GRAPHICS_RESOURCE(NewTypeName,VkObjTypeName)\
    VkObjectType NewTypeName::getObjectType() const { return VkObjTypeName; }\
    String NewTypeName::OBJ_TYPE_NAME = #VkObjTypeName; \
    const String& NewTypeName::getObjectTypeName() const { return OBJ_TYPE_NAME; }\
    DEFINE_GRAPHICS_RESOURCE(NewTypeName) 
#endif //DEFINE_VK_GRAPHICS_RESOURCE

#ifndef DEFINE_TEMPLATED_VK_GRAPHICS_RESOURCE
#define DEFINE_TEMPLATED_VK_GRAPHICS_RESOURCE(NewTypeName,NewTypeTemplates,TemplatesDefine,VkObjTypeName)\
    template ##NewTypeTemplates## \
    VkObjectType NewTypeName##TemplatesDefine##::getObjectType() const { return VkObjTypeName; }\
    \
    template ##NewTypeTemplates## \
    String NewTypeName##TemplatesDefine##::OBJ_TYPE_NAME = #VkObjTypeName; \
    \
    template ##NewTypeTemplates## \
    const String& NewTypeName##TemplatesDefine##::getObjectTypeName() const { return OBJ_TYPE_NAME; } \
    DEFINE_TEMPLATED_GRAPHICS_RESOURCE(NewTypeName,NewTypeTemplates,TemplatesDefine)
#endif //DEFINE_TEMPLATED_VK_GRAPHICS_RESOURCE
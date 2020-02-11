#pragma once


#ifndef CREATE_APP_INFO
#define CREATE_APP_INFO(VariableName)						\
VkApplicationInfo VariableName;								\
VariableName.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;	\
VariableName.pNext = nullptr
#endif

#ifndef CREATE_INSTANCE_INFO
#define CREATE_INSTANCE_INFO(VariableName)						\
VkInstanceCreateInfo VariableName;								\
VariableName.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;	\
VariableName.pNext = nullptr;									\
VariableName.flags = 0;											\
VariableName.ppEnabledLayerNames = nullptr;						\
VariableName.enabledLayerCount = 0
#endif

#ifndef CREATE_DEVICE_INFO
#define CREATE_DEVICE_INFO(VariableName)						\
VkDeviceCreateInfo VariableName;								\
VariableName.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;		\
VariableName.pNext = nullptr;									\
VariableName.flags = 0;											\
VariableName.enabledLayerCount = 0;								\
VariableName.ppEnabledLayerNames = nullptr						
#endif

#ifndef CREATE_QUEUE_INFO
#define CREATE_QUEUE_INFO(VariableName)							\
VkDeviceQueueCreateInfo VariableName;							\
VariableName.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;\
VariableName.pNext = nullptr;									\
VariableName.flags = 0
#endif


#ifndef CREATE_SWAPCHAIN_INFO
#define CREATE_SWAPCHAIN_INFO(VariableName)							\
VkSwapchainCreateInfoKHR VariableName;								\
VariableName.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;	\
VariableName.pNext = nullptr;										\
VariableName.flags = 0
#endif

#ifndef CREATE_DEBUG_UTILS_MESSENGER_INFO
#define CREATE_DEBUG_UTILS_MESSENGER_INFO(VariableName)							\
VkDebugUtilsMessengerCreateInfoEXT VariableName;								\
VariableName.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;	\
VariableName.pNext = nullptr;													\
VariableName.flags = 0;															\
VariableName.pUserData = nullptr
#endif


#ifndef PHYSICAL_DEVICE_FEATURES_2
#define PHYSICAL_DEVICE_FEATURES_2(VariableName)							\
VkPhysicalDeviceFeatures2KHR VariableName;									\
VariableName.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2_KHR;		\
VariableName.pNext = nullptr
#endif

#ifndef PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES
#define PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES(VariableName)						\
VkPhysicalDeviceTimelineSemaphoreFeaturesKHR VariableName;								\
VariableName.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES_KHR;	\
VariableName.pNext = nullptr
#endif

#ifndef PHYSICAL_DEVICE_PROPERTIES_2
#define PHYSICAL_DEVICE_PROPERTIES_2(VariableName)							\
VkPhysicalDeviceProperties2KHR VariableName;								\
VariableName.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2_KHR;	\
VariableName.pNext = nullptr
#endif

#ifndef PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_PROPERTIES
#define PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_PROPERTIES(VariableName)							\
VkPhysicalDeviceTimelineSemaphorePropertiesKHR VariableName;								\
VariableName.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_PROPERTIES_KHR;	\
VariableName.pNext = nullptr
#endif

#ifndef CREATE_SEMAPHORE_INFO
#define CREATE_SEMAPHORE_INFO(VariableName)						\
VkSemaphoreCreateInfo  VariableName;							\
VariableName.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;	\
VariableName.pNext = nullptr;									\
VariableName.flags = 0
#endif

#ifndef CREATE_TYPED_SEMAPHORE_INFO
#define CREATE_TYPED_SEMAPHORE_INFO(VariableName)						\
VkSemaphoreTypeCreateInfoKHR VariableName;								\
VariableName.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO_KHR;	\
VariableName.pNext = nullptr;											\
VariableName.initialValue = 0;											\
VariableName.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE_KHR
#endif

#ifndef SEMAPHORE_SIGNAL_INFO
#define SEMAPHORE_SIGNAL_INFO(VariableName)							\
VkSemaphoreSignalInfoKHR VariableName;								\
VariableName.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SIGNAL_INFO_KHR;	\
VariableName.pNext = nullptr
#endif


#ifndef SEMAPHORE_WAIT_INFO
#define SEMAPHORE_WAIT_INFO(VariableName)						\
VkSemaphoreWaitInfoKHR VariableName;							\
VariableName.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO_KHR;	\
VariableName.pNext = nullptr;									\
VariableName.flags = VK_SEMAPHORE_WAIT_ANY_BIT_KHR
#endif

#ifndef CREATE_FENCE_INFO
#define CREATE_FENCE_INFO(VariableName)						\
VkFenceCreateInfo VariableName;								\
VariableName.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;	\
VariableName.flags = 0;										\
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
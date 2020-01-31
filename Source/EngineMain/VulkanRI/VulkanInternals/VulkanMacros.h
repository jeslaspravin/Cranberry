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
VariableName.flags = 0
#endif

#ifndef CREATE_DEVICE_INFO
#define CREATE_DEVICE_INFO(VariableName)						\
VkDeviceCreateInfo VariableName;								\
VariableName.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;		\
VariableName.pNext = nullptr;									\
VariableName.flags = 0

#endif
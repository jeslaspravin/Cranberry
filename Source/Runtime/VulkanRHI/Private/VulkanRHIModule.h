#pragma once

#include "RenderInterface/IRHIModule.h"

class IVulkanRHIModule : public IRHIModule
{
public:
    virtual IGraphicsInstance* getGraphicsInstance() const = 0;

    static IVulkanRHIModule* get();
};
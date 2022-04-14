/*!
 * \file VulkanRHIModule.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "RenderInterface/IRHIModule.h"

class IVulkanRHIModule : public IRHIModule
{
public:
    virtual IGraphicsInstance *getGraphicsInstance() const = 0;

    static IVulkanRHIModule *get();
};
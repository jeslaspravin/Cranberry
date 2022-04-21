/*!
 * \file GenericVulkanFunctions.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

class ApplicationInstance;

template <typename... Parameters>
struct PFN_SurfaceKHR
{

public:
    virtual void setInstanceWindow(const ApplicationInstance *instance, const class GenericAppWindow *window) = 0;
    virtual void operator()(Parameters... params) const = 0;
};
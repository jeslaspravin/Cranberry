/*!
 * \file IImGuiLayer.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once
#include "Types/CoreTypes.h"

class ImGuiDrawInterface;

class IImGuiLayer
{
public:
    virtual int32 layerDepth() const = 0;
    virtual int32 sublayerDepth() const = 0;
    virtual void draw(class ImGuiDrawInterface *drawInterface) = 0;
};

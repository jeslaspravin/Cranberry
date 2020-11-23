#pragma once
#include "../../Core/Platform/PlatformTypes.h"

class ImGuiDrawInterface;

class IImGuiLayer
{
public:
    virtual int32 layerDepth() const = 0;
    virtual int32 sublayerDepth() const = 0;
    virtual void draw(class ImGuiDrawInterface* drawInterface) = 0;
};

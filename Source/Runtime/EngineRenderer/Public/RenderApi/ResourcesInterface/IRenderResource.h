/*!
 * \file IRenderResource.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "EngineRendererExports.h"
#include "Types/Containers/ReferenceCountPtr.h"

class GraphicsResource;
class MemoryResource;

/*
 * Interface wrappers to send graphics resources back into renderer interface
 */

// For MemoryResource
class ENGINERENDERER_EXPORT IRenderMemoryResource
{
public:
    virtual ReferenceCountPtr<MemoryResource> renderResource() const = 0;
};

class ENGINERENDERER_EXPORT IRenderTargetResource
{
public:
    virtual ReferenceCountPtr<MemoryResource> renderTargetResource() const = 0;
};

class ENGINERENDERER_EXPORT IRenderTargetTexture
    : public IRenderMemoryResource
    , public IRenderTargetResource
{};

// For general graphics resource
class ENGINERENDERER_EXPORT IRenderResource
{
public:
    virtual GraphicsResource *renderResource() const = 0;
};
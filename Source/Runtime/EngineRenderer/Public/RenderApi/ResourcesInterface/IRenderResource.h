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

class ENGINERENDERER_EXPORT IRenderTargetTexture : public IRenderMemoryResource, public IRenderTargetResource
{};

// For general graphics resource
class ENGINERENDERER_EXPORT IRenderResource
{
public:

    virtual MemoryResource* renderResource() const = 0;
};
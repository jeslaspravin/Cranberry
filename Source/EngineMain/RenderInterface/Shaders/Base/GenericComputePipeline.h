#pragma once

#include "../../PlatformIndependentHeaders.h"

class SimpleComputePipeline : public ComputePipeline
{
    DECLARE_GRAPHICS_RESOURCE(SimpleComputePipeline,, ComputePipeline,)

private:
    SimpleComputePipeline() = default;
public:
    SimpleComputePipeline(const PipelineBase* parent);
    SimpleComputePipeline(const ShaderResource* shaderResource);
};

using SimpleComputePipelineRegistrar = GenericPipelineRegistrar<SimpleComputePipeline>;

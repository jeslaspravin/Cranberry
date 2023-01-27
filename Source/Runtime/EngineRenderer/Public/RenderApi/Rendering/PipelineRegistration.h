/*!
 * \file PipelineRegistration.h
 *
 * \author Jeslas
 * \date June 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "Types/Delegates/Delegate.h"
#include "Types/Patterns/FactoriesBase.h"
#include "EngineRendererExports.h"

#include <map>

class PipelineBase;
class ShaderResource;
class String;
struct GraphicsPipelineConfig;
class IGraphicsInstance;
class GraphicsHelperAPI;
class StringID;

//////////////////////////////////////////////////////////////////////////
// PipelineFactory
//////////////////////////////////////////////////////////////////////////

struct PipelineFactoryArgs
{
    const ShaderResource *pipelineShader;
    const PipelineBase *parentPipeline = nullptr;
};

/*
 * Pipeline registration
 */
#define CREATE_GRAPHICS_PIPELINE_REGISTRANT(Registrant, ShaderName, FunctionPtr)                                                               \
    GraphicsPipelineFactoryRegistrant Registrant(                                                                                              \
        ShaderName, GraphicsPipelineFactoryRegistrant::GraphicsPipelineConfigGetter::createStatic(FunctionPtr)                                 \
    )

struct ENGINERENDERER_EXPORT GraphicsPipelineFactoryRegistrant
{
    using GraphicsPipelineConfigGetter = SingleCastDelegate<GraphicsPipelineConfig, String &, const ShaderResource *>;
    GraphicsPipelineConfigGetter getter;

public:
    GraphicsPipelineFactoryRegistrant(const TChar *shaderName, GraphicsPipelineConfigGetter configGetter);
    FORCE_INLINE PipelineBase *
    operator() (IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper, const PipelineFactoryArgs &args) const;
};

struct ENGINERENDERER_EXPORT ComputePipelineFactoryRegistrant
{
public:
    ComputePipelineFactoryRegistrant(const TChar *shaderName);
    FORCE_INLINE PipelineBase *
    operator() (IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper, const PipelineFactoryArgs &args) const;
};

class ENGINERENDERER_EXPORT PipelineFactory final
    : public FactoriesBase<PipelineBase *, IGraphicsInstance *, const GraphicsHelperAPI *, const PipelineFactoryArgs &>
{
private:
    friend GraphicsPipelineFactoryRegistrant;
    friend ComputePipelineFactoryRegistrant;

    static std::map<StringID, GraphicsPipelineFactoryRegistrant> &graphicsPipelineFactoriesRegistry();
    static std::map<StringID, ComputePipelineFactoryRegistrant> &computePipelineFactoriesRegistry();

public:
    PipelineBase *
    create(IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper, const PipelineFactoryArgs &args) const final;
};
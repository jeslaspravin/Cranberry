/*!
 * \file Pipelines.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */


#include "Types/Platform/PlatformAssertionErrors.h"
#include "Types/Platform/LFS/PlatformLFS.h"
#include "RenderInterface/Resources/Pipelines.h"
#include "RenderInterface/Resources/ShaderResources.h"
#include "RenderInterface/GraphicsHelper.h"
#include "RenderInterface/Shaders/Base/DrawMeshShader.h"
#include "RenderInterface/Shaders/Base/UtilityShaders.h"

DEFINE_GRAPHICS_RESOURCE(PipelineCacheBase)

std::vector<uint8> PipelineCacheBase::getRawFromFile() const
{
    PlatformFile cacheFile(cacheFileName);
    cacheFile.setSharingMode(EFileSharing::ReadOnly);
    cacheFile.setFileFlags(EFileFlags::Read | EFileFlags::OpenExisting);

    std::vector<uint8> cacheData;
    if (cacheFile.exists() && cacheFile.openFile())
    {
        cacheFile.read(cacheData);
        cacheFile.closeFile();
    }
    return cacheData;
}

String PipelineCacheBase::getResourceName() const
{
    return cacheName;
}
void PipelineCacheBase::setResourceName(const String& name)
{
    cacheName = name;
    cacheFileName = PathFunctions::combinePath(FileSystemFunctions::applicationDirectory(cacheFileName), "Cache", cacheName + ".cache");
}

void PipelineCacheBase::addPipelineToCache(const class PipelineBase* pipeline)
{
    pipelinesToCache.emplace_back(pipeline);
}

void PipelineCacheBase::writeCache() const
{
    PlatformFile cacheFile(cacheFileName);
    cacheFile.setSharingMode(EFileSharing::NoSharing);
    cacheFile.setFileFlags(EFileFlags::Write | EFileFlags::CreateAlways);

    cacheFile.openOrCreate();
    std::vector<uint8> pipelineCacheData = getRawToWrite();
    cacheFile.write({ pipelineCacheData });
    cacheFile.closeFile();
}

//////////////////////////////////////////////////////////////////////////
// Pipeline resource
//////////////////////////////////////////////////////////////////////////

DEFINE_GRAPHICS_RESOURCE(PipelineBase)

PipelineBase::PipelineBase(const PipelineBase* parent)
    : pipelineName(parent->pipelineName)
    , bCanBeParent(false)
    , parentPipeline(parent)
    , parentCache(parent->parentCache)
    , pipelineShader(parent->pipelineShader)
    , shaderParamLayouts(parent->shaderParamLayouts)
{}

String PipelineBase::getResourceName() const
{
    return pipelineName;
}

void PipelineBase::setResourceName(const String& name)
{
    pipelineName = name;
}

void PipelineBase::setParentPipeline(const PipelineBase* parent)
{
    parentPipeline = parent;
}

void PipelineBase::setParamLayoutAtSet(const GraphicsResource* paramLayout, int32 setIdx /*= -1*/)
{
    if (setIdx < 0)
    {
        shaderParamLayouts.clear();
        shaderParamLayouts.emplace_back(paramLayout);
    }
    else
    {
        if (shaderParamLayouts.size() <= setIdx)
        {
            shaderParamLayouts.resize(setIdx + 1);
        }
        shaderParamLayouts[setIdx] = paramLayout;
    }
}

void PipelineBase::setPipelineCache(const PipelineCacheBase* pipelineCache)
{
    parentCache = pipelineCache;
}

const GraphicsResource* PipelineBase::getParamLayoutAtSet(int32 setIdx) const
{
    return shaderParamLayouts[setIdx];
}

//////////////////////////////////////////////////////////////////////////
/// Graphics pipeline
//////////////////////////////////////////////////////////////////////////

DEFINE_GRAPHICS_RESOURCE(GraphicsPipelineBase)

GraphicsPipelineBase::GraphicsPipelineBase(const GraphicsPipelineBase* parent)
    : PipelineBase(parent)
    , config(parent->config)
{}

GraphicsPipelineQueryParams GraphicsPipelineBase::paramForIdx(int32 idx) const
{
    GraphicsPipelineQueryParams queryParam;

    int32 denominator = pipelinesCount();
    int32 numerator = idx;

    // Highest degree to lowest. For each degree we divide max number of elements at/below that degree 
    // with max elements at that degree to get multiplier. The multiplier is then used to divide the 
    // remaining total elements to find the index in current option. After that the multiplier is used to 
    // find remaining encoded element by doing modulo
     
    // Draw mode
    denominator /= int32(config.allowedDrawModes.size());
    queryParam.drawMode = config.allowedDrawModes[numerator / denominator];
    numerator %= denominator;

    // Culling mode
    denominator /= int32(config.supportedCullings.size());
    queryParam.cullingMode = config.supportedCullings[numerator / denominator];
    //numerator %= denominator;

    return queryParam;
}

int32 GraphicsPipelineBase::idxFromParam(GraphicsPipelineQueryParams queryParam) const
{
    int32 idx = 0;

    int32 polyDeg = pipelinesCount();
    int32 tempIdx = 0;

    // Highest degree to lowest. For each degree we divide max number of elements at/below that degree 
    // with max elements at that degree to get multiplier. The multiplier is then multiplied to current option's index
    // to find value at this degree and added to final index.

    // Draw mode
    polyDeg /= int32(config.allowedDrawModes.size());
    while (tempIdx < config.allowedDrawModes.size() && config.allowedDrawModes[tempIdx] != queryParam.drawMode) ++tempIdx;
    if (tempIdx >= config.allowedDrawModes.size())
    {
        Logger::warn("GraphicsPipeline", "%s() : Not supported draw mode %d for pipeline of shader %s"
            , __func__, tempIdx, pipelineShader->getResourceName().getChar());
    }
    idx += (tempIdx % config.allowedDrawModes.size()) * polyDeg;

    // Culling mode
    polyDeg /= int32(config.supportedCullings.size());
    tempIdx = 0;
    while (tempIdx < config.supportedCullings.size() && config.supportedCullings[tempIdx] != queryParam.cullingMode) ++tempIdx;
    if (tempIdx >= config.supportedCullings.size())
    {
        Logger::warn("GraphicsPipeline", "%s() : Not supported culling mode %d for pipeline of shader %s"
            , __func__, tempIdx, pipelineShader->getResourceName().getChar());
    }
    idx += (tempIdx % config.supportedCullings.size()) * polyDeg;

    return idx;
}

//////////////////////////////////////////////////////////////////////////
/// Compute pipeline
//////////////////////////////////////////////////////////////////////////

DEFINE_GRAPHICS_RESOURCE(ComputePipelineBase)

ComputePipelineBase::ComputePipelineBase(const ComputePipelineBase* parent)
    : BaseType(parent)
{}

//////////////////////////////////////////////////////////////////////////
// PipelineFactory
//////////////////////////////////////////////////////////////////////////

GraphicsPipelineFactoryRegistrant::GraphicsPipelineFactoryRegistrant(const String& shaderName, GraphicsPipelineConfigGetter configGetter) 
    : getter(configGetter)
{
    PipelineFactory::graphicsPipelineFactoriesRegistry().insert({ shaderName , *this });
}

FORCE_INLINE PipelineBase* GraphicsPipelineFactoryRegistrant::operator()(IGraphicsInstance* graphicsInstance, const GraphicsHelperAPI* graphicsHelper, const PipelineFactoryArgs& args) const
{
    PipelineBase* pipeline;
    if (args.parentPipeline != nullptr)
    {
        pipeline = graphicsHelper->createGraphicsPipeline(graphicsInstance, args.parentPipeline);
    }
    else 
    {
        fatalAssert(getter.isBound(), "%s() : Invalid GraphicsPipelineConfig getter for shader %s", __func__, args.pipelineShader->getResourceName().getChar());
        String pipelineName;
        pipeline = graphicsHelper->createGraphicsPipeline(graphicsInstance, getter.invoke(pipelineName, args.pipelineShader));
        pipeline->setResourceName(pipelineName);
        pipeline->setPipelineShader(args.pipelineShader);
    }
    return pipeline;
}

ComputePipelineFactoryRegistrant::ComputePipelineFactoryRegistrant(const String& shaderName)
{
    PipelineFactory::computePipelineFactoriesRegistry().insert({ shaderName , *this });
}

FORCE_INLINE PipelineBase* ComputePipelineFactoryRegistrant::operator()(IGraphicsInstance* graphicsInstance, const GraphicsHelperAPI* graphicsHelper, const PipelineFactoryArgs& args) const
{
    PipelineBase* pipeline;
    if (args.parentPipeline != nullptr)
    {
        pipeline = graphicsHelper->createGraphicsPipeline(graphicsInstance, args.parentPipeline);
    }
    else
    {
        String pipelineName = "Compute_" + args.pipelineShader->getResourceName();
        pipeline = graphicsHelper->createComputePipeline(graphicsInstance);
        pipeline->setResourceName(pipelineName);
        pipeline->setPipelineShader(args.pipelineShader);
    }
    return pipeline;
}

std::map<String, GraphicsPipelineFactoryRegistrant>& PipelineFactory::graphicsPipelineFactoriesRegistry()
{
    static std::map<String, GraphicsPipelineFactoryRegistrant> REGISTERED_PIPELINE_FACTORIES;
    return REGISTERED_PIPELINE_FACTORIES;
}

std::map<String, ComputePipelineFactoryRegistrant>& PipelineFactory::computePipelineFactoriesRegistry()
{
    static std::map<String, ComputePipelineFactoryRegistrant> REGISTERED_PIPELINE_FACTORIES;
    return REGISTERED_PIPELINE_FACTORIES;
}

PipelineBase* PipelineFactory::create(IGraphicsInstance* graphicsInstance, const GraphicsHelperAPI* graphicsHelper, const PipelineFactoryArgs& args) const
{
    fatalAssert(args.pipelineShader, "Pipeline shader cannot be null");
    if (args.pipelineShader->getShaderConfig()->getType()->isChildOf<DrawMeshShaderConfig>() 
        || args.pipelineShader->getShaderConfig()->getType()->isChildOf<UniqueUtilityShaderConfig>())
    {
        auto factoryItr = graphicsPipelineFactoriesRegistry().find(args.pipelineShader->getResourceName());
        fatalAssert(factoryItr != graphicsPipelineFactoriesRegistry().end(), "Failed finding factory to create graphics pipeline for shader %s", args.pipelineShader->getResourceName().getChar());

        return (factoryItr->second)(graphicsInstance, graphicsHelper, args);
    }
    else if(args.pipelineShader->getShaderConfig()->getType()->isChildOf<ComputeShaderConfig>())
    {
        auto factoryItr = computePipelineFactoriesRegistry().find(args.pipelineShader->getResourceName());
        fatalAssert(factoryItr != computePipelineFactoriesRegistry().end(), "Failed finding factory to create compute pipeline for shader %s", args.pipelineShader->getResourceName().getChar());

        return (factoryItr->second)(graphicsInstance, graphicsHelper, args);
    }
    Logger::error("PipelineFactory", "%() : Pipeline factory unsupported shader config/shader", __func__);
    return nullptr;
}
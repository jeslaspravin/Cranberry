/*!
 * \file Pipelines.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "RenderInterface/Resources/Pipelines.h"
#include "RenderInterface/GraphicsHelper.h"
#include "RenderInterface/Resources/ShaderResources.h"
#include "RenderApi/Shaders/Base/DrawMeshShader.h"
#include "RenderApi/Shaders/Base/UtilityShaders.h"
#include "Types/Platform/LFS/PlatformLFS.h"
#include "Types/Platform/LFS/PathFunctions.h"
#include "Types/Platform/LFS/Paths.h"
#include "Types/Platform/PlatformAssertionErrors.h"

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

String PipelineCacheBase::getResourceName() const { return cacheName; }
void PipelineCacheBase::setResourceName(const String &name)
{
    cacheName = name;
    cacheFileName = PathFunctions::combinePath(Paths::savedDirectory(), TCHAR("Cache"), Paths::applicationName() + cacheName + TCHAR(".cache"));
}

void PipelineCacheBase::addPipelineToCache(const PipelineBase *pipeline) { pipelinesToCache.emplace_back(pipeline); }

void PipelineCacheBase::writeCache() const
{
    PlatformFile cacheFile(cacheFileName);
    cacheFile.setSharingMode(EFileSharing::NoSharing);
    cacheFile.setFileFlags(EFileFlags::Write | EFileFlags::CreateAlways);

    cacheFile.openOrCreate();
    std::vector<uint8> pipelineCacheData = getRawToWrite();
    cacheFile.write({ pipelineCacheData.data(), pipelineCacheData.size() });
    cacheFile.closeFile();
}

//////////////////////////////////////////////////////////////////////////
// Pipeline resource
//////////////////////////////////////////////////////////////////////////

DEFINE_GRAPHICS_RESOURCE(PipelineBase)

PipelineBase::PipelineBase(const PipelineBase *parent)
    : pipelineName(parent->pipelineName)
    , bCanBeParent(false)
    , parentPipeline(parent)
    , parentCache(parent->parentCache)
    , pipelineShader(parent->pipelineShader)
    , shaderParamLayouts(parent->shaderParamLayouts)
{}

String PipelineBase::getResourceName() const { return pipelineName; }

void PipelineBase::setResourceName(const String &name) { pipelineName = name; }

void PipelineBase::setParentPipeline(const PipelineBase *parent) { parentPipeline = parent; }

void PipelineBase::setParamLayoutAtSet(const GraphicsResource *paramLayout, int32 setIdx /*= -1*/)
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

void PipelineBase::setPipelineCache(const PipelineCacheBase *pipelineCache) { parentCache = pipelineCache; }

const GraphicsResource *PipelineBase::getParamLayoutAtSet(int32 setIdx) const { return shaderParamLayouts[setIdx]; }

//////////////////////////////////////////////////////////////////////////
/// Graphics pipeline
//////////////////////////////////////////////////////////////////////////

DEFINE_GRAPHICS_RESOURCE(GraphicsPipelineBase)

GraphicsPipelineBase::GraphicsPipelineBase(const GraphicsPipelineBase *parent)
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
    // numerator %= denominator;

    return queryParam;
}

int32 GraphicsPipelineBase::idxFromParam(GraphicsPipelineQueryParams queryParam) const
{
    int32 idx = 0;

    int32 polyDeg = pipelinesCount();
    int32 tempIdx = 0;

    // Highest degree to lowest. For each degree we divide max number of elements at/below that degree
    // with max elements at that degree to get multiplier. The multiplier is then multiplied to current
    // option's index to find value at this degree and added to final index.

    // Draw mode
    polyDeg /= int32(config.allowedDrawModes.size());
    while (tempIdx < config.allowedDrawModes.size() && config.allowedDrawModes[tempIdx] != queryParam.drawMode)
    {
        ++tempIdx;
    }
    if (tempIdx >= config.allowedDrawModes.size())
    {
        LOG_WARN(
            "GraphicsPipeline", "Not supported draw mode {} for pipeline of shader {}", tempIdx, pipelineShader->getResourceName().getChar()
        );
    }
    idx += (tempIdx % config.allowedDrawModes.size()) * polyDeg;

    // Culling mode
    polyDeg /= int32(config.supportedCullings.size());
    tempIdx = 0;
    while (tempIdx < config.supportedCullings.size() && config.supportedCullings[tempIdx] != queryParam.cullingMode)
    {
        ++tempIdx;
    }
    if (tempIdx >= config.supportedCullings.size())
    {
        LOG_WARN(
            "GraphicsPipeline", "Not supported culling mode {} for pipeline of shader {}", tempIdx, pipelineShader->getResourceName().getChar()
        );
    }
    idx += (tempIdx % config.supportedCullings.size()) * polyDeg;

    return idx;
}

//////////////////////////////////////////////////////////////////////////
/// Compute pipeline
//////////////////////////////////////////////////////////////////////////

DEFINE_GRAPHICS_RESOURCE(ComputePipelineBase)

ComputePipelineBase::ComputePipelineBase(const ComputePipelineBase *parent)
    : BaseType(parent)
{}
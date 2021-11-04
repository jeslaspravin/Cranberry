#include "Pipelines.h"
#include "ShaderResources.h"
#include "../../Core/Platform/PlatformAssertionErrors.h"
#include "../../Core/Platform/LFS/PlatformLFS.h"

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
    cacheFileName = FileSystemFunctions::combinePath(FileSystemFunctions::applicationDirectory(cacheFileName), "Cache", cacheName + ".cache");
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
    , renderpassProps(parent->renderpassProps)
    , primitiveTopology(parent->primitiveTopology)
    , cntrlPts(parent->cntrlPts)
    //, bEnableDepthBias(parent->bEnableDepthBias)
    , depthState(parent->depthState)
    , stencilStateFront(parent->stencilStateFront)
    , stencilStateBack(parent->stencilStateBack)
    , attachmentBlendStates(parent->attachmentBlendStates)
    , allowedDrawModes(parent->allowedDrawModes)
    , supportedCullings(parent->supportedCullings)
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
    denominator /= int32(allowedDrawModes.size());
    queryParam.drawMode = allowedDrawModes[numerator / denominator];
    numerator %= denominator;

    // Culling mode
    denominator /= int32(supportedCullings.size());
    queryParam.cullingMode = supportedCullings[numerator / denominator];
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
    polyDeg /= int32(allowedDrawModes.size());
    while (tempIdx < allowedDrawModes.size() && allowedDrawModes[tempIdx] != queryParam.drawMode) ++tempIdx;
    if (tempIdx >= allowedDrawModes.size())
    {
        Logger::warn("GraphicsPipeline", "%s() : Not supported draw mode %d for pipeline of shader %s"
            , __func__, tempIdx, pipelineShader->getResourceName().getChar());
    }
    idx += (tempIdx % allowedDrawModes.size()) * polyDeg;

    // Culling mode
    polyDeg /= int32(supportedCullings.size());
    tempIdx = 0;
    while (tempIdx < supportedCullings.size() && supportedCullings[tempIdx] != queryParam.cullingMode) ++tempIdx;
    if (tempIdx >= supportedCullings.size())
    {
        Logger::warn("GraphicsPipeline", "%s() : Not supported culling mode %d for pipeline of shader %s"
            , __func__, tempIdx, pipelineShader->getResourceName().getChar());
    }
    idx += (tempIdx % supportedCullings.size()) * polyDeg;

    return idx;
}

FORCE_INLINE int32 GraphicsPipelineBase::pipelinesCount() const
{
    return int32(allowedDrawModes.size() * supportedCullings.size());
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

PipelineFactoryRegistrar::PipelineFactoryRegistrar(const String& shaderName)
{
    PipelineFactory::namedPipelineFactoriesRegistry().insert({ shaderName , this });
}

std::map<String, const PipelineFactoryRegistrar*>& PipelineFactory::namedPipelineFactoriesRegistry()
{
    static std::map<String, const PipelineFactoryRegistrar*> REGISTERED_PIPELINE_FACTORIES;
    return REGISTERED_PIPELINE_FACTORIES;
}

PipelineBase* PipelineFactory::create(const PipelineFactoryArgs& args) const
{
    fatalAssert(args.pipelineShader, "Pipeline shader cannot be null");
    auto factoryItr = namedPipelineFactoriesRegistry().find(args.pipelineShader->getResourceName());
    fatalAssert(factoryItr != namedPipelineFactoriesRegistry().end(), "Failed finding factory to create pipeline for shader %s", args.pipelineShader->getResourceName().getChar());

    return (*factoryItr->second)(args);
}

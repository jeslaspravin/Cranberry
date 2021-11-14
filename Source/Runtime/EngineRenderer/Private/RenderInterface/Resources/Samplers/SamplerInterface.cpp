#include "RenderInterface/Resources/Samplers/SamplerInterface.h"

#include <sstream>

DEFINE_GRAPHICS_RESOURCE(SamplerInterface)

SamplerInterface::SamplerInterface(SamplerCreateInfo samplerCI)
    : BaseType()
    , config(samplerCI)
{
    std::stringstream nameStream("Sampler_");
    nameStream << ESamplerFiltering::filterName(config.filtering).getChar() << "_";
    nameStream << std::get<0>(config.tilingMode) << "_";
    nameStream << std::get<1>(config.tilingMode) << "_";
    nameStream << std::get<2>(config.tilingMode);
    config.resourceName = nameStream.str();
}

void SamplerInterface::setMipLod(const float& fineMipLod, const float& poorMipLod)
{
    config.mipLodRange.minBound = fineMipLod;
    config.mipLodRange.maxBound = poorMipLod;
}

void SamplerInterface::getMipLod(float& fineMipLod, float& poorMipLod)
{
    fineMipLod = config.mipLodRange.minBound;
    poorMipLod = config.mipLodRange.maxBound;
}

void SamplerInterface::setMipFiltering(ESamplerFiltering::Type samplerFiltering)
{
    config.mipFiltering = samplerFiltering;
}

ESamplerFiltering::Type SamplerInterface::getMipFiltering()
{
    return config.mipFiltering;
}

ESamplerFiltering::Type SamplerInterface::getFinestFiltering()
{
    return config.filtering;
}

void SamplerInterface::setCompareOp(bool enable, CoreGraphicsTypes::ECompareOp::Type compareOpValue)
{
    config.useCompareOp = enable ? 1 : 0;
    config.compareOp = compareOpValue;
}

bool SamplerInterface::getCompareOp(CoreGraphicsTypes::ECompareOp::Type& compareOpValue)
{
    compareOpValue = config.compareOp;
    return config.useCompareOp;
}

void SamplerInterface::setBorderColor(uint8 samplerBorderColFlags)
{
    config.borderColorFlags = samplerBorderColFlags;
}

void SamplerInterface::setTilingMode(ESamplerTilingMode::Type u, ESamplerTilingMode::Type v, ESamplerTilingMode::Type w)
{
    std::get<0>(config.tilingMode) = u;
    std::get<1>(config.tilingMode) = v; 
    std::get<2>(config.tilingMode) = w;
}

void SamplerInterface::getTilingMode(ESamplerTilingMode::Type& u, ESamplerTilingMode::Type& v, ESamplerTilingMode::Type& w)
{
    u = std::get<0>(config.tilingMode);
    v = std::get<1>(config.tilingMode);
    w = std::get<2>(config.tilingMode);
}

void SamplerInterface::addRef()
{
    refCounter.fetch_add(1);
}

void SamplerInterface::removeRef()
{
    uint32 count = refCounter.fetch_sub(1);
    if (count == 1)
    {
        release();
        delete this;
    }
}

uint32 SamplerInterface::refCount() const
{
    return refCounter.load();
}

String SamplerInterface::getResourceName() const
{
    return config.resourceName;
}

void SamplerInterface::setResourceName(const String& name)
{
    config.resourceName = name;
}

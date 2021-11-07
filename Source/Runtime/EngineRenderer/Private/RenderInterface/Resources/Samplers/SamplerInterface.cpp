#include "RenderInterface/Resources/Samplers/SamplerInterface.h"

#include <sstream>

DEFINE_GRAPHICS_RESOURCE(SamplerInterface)

SamplerInterface::SamplerInterface()
    : BaseType()
    , filtering(ESamplerFiltering::Nearest)
    , mipFiltering(ESamplerFiltering::Nearest)
    , tilingMode(0,0,0)
    , mipLodRange(0,0)
    , compareOp(CoreGraphicsTypes::ECompareOp::Greater)
    , useCompareOp(0)
    , borderColorFlags(0)
{}

SamplerInterface::SamplerInterface(ESamplerTilingMode::Type samplerTiling, ESamplerFiltering::Type samplerFiltering,
    float poorMipLod /*= 0*/, uint8 samplerBorderColFlags /*= 0*/)
    : BaseType()
    , filtering(samplerFiltering)
    , mipFiltering(samplerFiltering)
    , tilingMode(samplerTiling,samplerTiling,samplerTiling)
    , mipLodRange(0, poorMipLod)
    , compareOp(CoreGraphicsTypes::ECompareOp::Greater)
    , useCompareOp(0)
    , borderColorFlags(samplerBorderColFlags)
{
    std::stringstream nameStream("Sampler_");
    nameStream << ESamplerFiltering::getFilterInfo(samplerFiltering)->filterName.getChar() << "_";
    nameStream << ESamplerTilingMode::getSamplerTiling(samplerTiling);
    resourceName = nameStream.str();
}

void SamplerInterface::setMipLod(const float& fineMipLod, const float& poorMipLod)
{
    mipLodRange.x = fineMipLod;
    mipLodRange.y = poorMipLod;
}

void SamplerInterface::getMipLod(float& fineMipLod, float& poorMipLod)
{
    fineMipLod = mipLodRange.x;
    poorMipLod = mipLodRange.y;
}

void SamplerInterface::setMipFiltering(ESamplerFiltering::Type samplerFiltering)
{
    mipFiltering = samplerFiltering;
}

ESamplerFiltering::Type SamplerInterface::getMipFiltering()
{
    return mipFiltering;
}

ESamplerFiltering::Type SamplerInterface::getFinestFiltering()
{
    return filtering;
}

void SamplerInterface::setCompareOp(bool enable, CoreGraphicsTypes::ECompareOp::Type compareOpValue)
{
    useCompareOp = enable ? 1 : 0;
    compareOp = compareOpValue;
}

bool SamplerInterface::getCompareOp(CoreGraphicsTypes::ECompareOp::Type& compareOpValue)
{
    compareOpValue = compareOp;
    return useCompareOp;
}

void SamplerInterface::setBorderColor(uint8 samplerBorderColFlags)
{
    borderColorFlags = samplerBorderColFlags;
}

void SamplerInterface::setTilingMode(ESamplerTilingMode::Type u, ESamplerTilingMode::Type v, ESamplerTilingMode::Type w)
{
    tilingMode.x = u; tilingMode.y = v; tilingMode.z = w;
}

void SamplerInterface::getTilingMode(ESamplerTilingMode::Type& u, ESamplerTilingMode::Type& v, ESamplerTilingMode::Type& w)
{
    u = tilingMode.x; v = tilingMode.y; w = tilingMode.z;
}

String SamplerInterface::getResourceName() const
{
    return resourceName;
}

void SamplerInterface::setResourceName(const String& name)
{
    resourceName = name;
}

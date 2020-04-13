#pragma once
#include "GraphicsResources.h"
#include "../../Core/Math/CoreMathTypedefs.h"
#include "../../Core/String/String.h"
#include "../../Core/Types/HashTypes.h"
#include "../CoreGraphicsTypes.h"

struct BufferViewInfo
{
    uint64 startOffset = 0;
    uint64 size = (~0ULL);/* VK_WHOLE_SIZE */

    bool operator<(const BufferViewInfo& otherViewInfo) const
    {
        return size < otherViewInfo.size;
    }

    bool operator==(const BufferViewInfo& otherViewInfo) const
    {
        return size == otherViewInfo.size && startOffset == otherViewInfo.startOffset;
    }
};

template <>
struct std::hash<BufferViewInfo>
{
    _NODISCARD size_t operator()(const BufferViewInfo& keyval) const noexcept {
        hash<uint64> uint64Hasher;
        size_t seed = uint64Hasher(keyval.startOffset);
        HashUtility::hashCombine(seed, uint64Hasher(keyval.size));
        return seed;
    }
};

struct ImageViewInfo
{
    struct ImageComponentMapping
    {
        EImageComponentMapping::Type r = EImageComponentMapping::SameComponent;
        EImageComponentMapping::Type g = EImageComponentMapping::SameComponent;
        EImageComponentMapping::Type b = EImageComponentMapping::SameComponent;
        EImageComponentMapping::Type a = EImageComponentMapping::SameComponent;

        bool operator==(const ImageComponentMapping& otherCompMapping) const
        {
            return r == otherCompMapping.r && g == otherCompMapping.g && b == otherCompMapping.b && a == otherCompMapping.a;
        }
    };

    struct ImageSubresource
    {
        uint32 baseMip = 0;
        uint32 mipCount = (~0U); /* VK_REMAINING_MIP_LEVELS */
        uint32 baseLayer = 0;
        uint32 layersCount = (~0U);

        bool operator<(const ImageSubresource& otherSubresource) const
        {
            return layersCount == otherSubresource.layersCount
                ? mipCount < otherSubresource.mipCount
                : layersCount < otherSubresource.layersCount;
        }

        bool operator==(const ImageSubresource& otherSubresource) const
        {
            return baseLayer == otherSubresource.baseLayer && baseMip == otherSubresource.baseMip
                && mipCount == otherSubresource.mipCount && layersCount == otherSubresource.layersCount;
        }
    };

    ImageComponentMapping componentMapping;
    ImageSubresource viewSubresource;

    // Used only in case of depth and stencil textures
    bool bUseStencil = false;

    bool operator<(const ImageViewInfo& otherViewInfo) const
    {
        return viewSubresource < otherViewInfo.viewSubresource;
    }

    bool operator==(const ImageViewInfo& otherViewInfo) const
    {
        return bUseStencil == otherViewInfo.bUseStencil && componentMapping == otherViewInfo.componentMapping
            && viewSubresource == otherViewInfo.viewSubresource;
    }
};
template <>
struct std::hash<ImageViewInfo::ImageComponentMapping>
{
    _NODISCARD size_t operator()(const ImageViewInfo::ImageComponentMapping& keyval) const noexcept {
        hash<uint32> uint32Hasher;
        size_t seed = uint32Hasher(keyval.r);
        HashUtility::hashCombine(seed, keyval.g);
        HashUtility::hashCombine(seed, keyval.b);
        HashUtility::hashCombine(seed, keyval.a);
        return seed;
    }
};

template <>
struct std::hash<ImageViewInfo::ImageSubresource>
{
    _NODISCARD size_t operator()(const ImageViewInfo::ImageSubresource& keyval) const noexcept {
        hash<uint32> uint32Hasher;
        size_t seed = uint32Hasher(keyval.baseLayer);
        HashUtility::hashCombine(seed, keyval.baseMip);
        HashUtility::hashCombine(seed, keyval.layersCount);
        HashUtility::hashCombine(seed, keyval.mipCount);
        return seed;
    }
};

template <>
struct std::hash<ImageViewInfo>
{
    _NODISCARD size_t operator()(const ImageViewInfo& keyval) const noexcept {
        size_t seed = hash<bool>{}(keyval.bUseStencil);
        HashUtility::hashCombine(seed, keyval.componentMapping);
        HashUtility::hashCombine(seed, keyval.viewSubresource);
        return seed;
    }
};


class MemoryResource : public GraphicsResource
{
    DECLARE_GRAPHICS_RESOURCE(MemoryResource, , GraphicsResource, )

protected:
    // For image this is always used for buffer this is used only in special cases
    EPixelDataFormat::Type dataFormat;
    bool bIsStagingResource;

    MemoryResource(EPixelDataFormat::Type resourceFormat = EPixelDataFormat::Undefined) 
        : BaseType()
        , dataFormat(resourceFormat)
        , bIsStagingResource(false)
    {}

    String memoryResName;

public:
    virtual uint64 getResourceSize() const { return 0; }
    virtual bool isValid() { return false; }

    bool isStagingResource() const { return bIsStagingResource; }

    /* GraphicsResource overrides */
    String getResourceName() const override;
    void setResourceName(const String& name) override;
    /* overrides ends */
};

class BufferResource : public MemoryResource
{
    DECLARE_GRAPHICS_RESOURCE(BufferResource, , MemoryResource, )

public:
    // TODO(Jeslas) : Check if this is needed later point of development
    void setAsStagingResource(bool isStaging) { bIsStagingResource = isStaging; }
};

class ImageResource : public MemoryResource
{
    DECLARE_GRAPHICS_RESOURCE(ImageResource, , MemoryResource, )

protected:
    Size3D dimensions = { 256,256,1 };
    uint32 numOfMips = 0;
    uint32 layerCount = 1;
    EPixelSampleCount::Type sampleCounts = EPixelSampleCount::SampleCount1;
    uint32 shaderUsage = EImageShaderUsage::Sampling;
    bool isRenderTarget = false;

    ImageResource() = default;
public:
    ImageResource(EPixelDataFormat::Type imageFormat);

    void setLayerCount(uint32 count);
    uint32 getLayerCount() const { return layerCount; }

    void setSampleCounts(EPixelSampleCount::Type samples) { sampleCounts = samples; }
    void setNumOfMips(uint32 mipCount) { numOfMips = mipCount; }
    uint32 getNumOfMips() const { return numOfMips; };

    void setShaderUsage(uint32 usage) { shaderUsage = usage; }
    void setImageSize(const Size3D& imageSize) { dimensions = imageSize; }
    const Size3D& getImageSize() const { return dimensions; }
};
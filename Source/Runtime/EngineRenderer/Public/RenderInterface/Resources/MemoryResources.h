/*!
 * \file MemoryResources.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once
#include <atomic>

#include "GraphicsResources.h"
#include "Math/CoreMathTypedefs.h"
#include "String/String.h"
#include "Types/HashTypes.h"
#include "Types/CoreDefines.h"
#include "RenderInterface/CoreGraphicsTypes.h"
#include "Types/Containers/ReferenceCountPtr.h"

struct ENGINERENDERER_EXPORT BufferViewInfo
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
struct ENGINERENDERER_EXPORT std::hash<BufferViewInfo>
{
    _NODISCARD size_t operator()(const BufferViewInfo& keyval) const noexcept {
        hash<uint64> uint64Hasher;
        size_t seed = uint64Hasher(keyval.startOffset);
        HashUtility::hashCombine(seed, uint64Hasher(keyval.size));
        return seed;
    }
};


struct ENGINERENDERER_EXPORT ImageSubresource
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

struct ENGINERENDERER_EXPORT ImageViewInfo
{
    struct ImageComponentMapping
    {
        EPixelComponentMapping::Type r = EPixelComponentMapping::SameComponent;
        EPixelComponentMapping::Type g = EPixelComponentMapping::SameComponent;
        EPixelComponentMapping::Type b = EPixelComponentMapping::SameComponent;
        EPixelComponentMapping::Type a = EPixelComponentMapping::SameComponent;

        bool operator==(const ImageComponentMapping& otherCompMapping) const
        {
            return r == otherCompMapping.r && g == otherCompMapping.g && b == otherCompMapping.b && a == otherCompMapping.a;
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
using ImageViewTypeAndInfo = std::pair<int32, ImageViewInfo>;

template <>
struct ENGINERENDERER_EXPORT std::hash<ImageViewInfo::ImageComponentMapping>
{
    _NODISCARD size_t operator()(const ImageViewInfo::ImageComponentMapping& keyval) const noexcept 
    {
        hash<uint32> uint32Hasher;
        size_t seed = uint32Hasher(keyval.r);
        HashUtility::hashCombine(seed, keyval.g);
        HashUtility::hashCombine(seed, keyval.b);
        HashUtility::hashCombine(seed, keyval.a);
        return seed;
    }
};

template <>
struct ENGINERENDERER_EXPORT std::hash<ImageSubresource>
{
    _NODISCARD size_t operator()(const ImageSubresource& keyval) const noexcept 
    {
        hash<uint32> uint32Hasher;
        size_t seed = uint32Hasher(keyval.baseLayer);
        HashUtility::hashCombine(seed, keyval.baseMip);
        HashUtility::hashCombine(seed, keyval.layersCount);
        HashUtility::hashCombine(seed, keyval.mipCount);
        return seed;
    }
};

template <>
struct ENGINERENDERER_EXPORT std::hash<ImageViewInfo>
{
    _NODISCARD size_t operator()(const ImageViewInfo& keyval) const noexcept 
    {
        size_t seed = hash<bool>{}(keyval.bUseStencil);
        HashUtility::hashCombine(seed, keyval.componentMapping);
        HashUtility::hashCombine(seed, keyval.viewSubresource);
        return seed;
    }
};

template <>
struct ENGINERENDERER_EXPORT std::hash<ImageViewTypeAndInfo>
{
    _NODISCARD size_t operator()(const ImageViewTypeAndInfo& keyval) const noexcept 
    {
        size_t seed = hash<int32>{}(keyval.first);
        HashUtility::hashCombine(seed, keyval.second);
        return seed;
    }
};


class ENGINERENDERER_EXPORT MemoryResource : public GraphicsResource
{
    DECLARE_GRAPHICS_RESOURCE(MemoryResource, , GraphicsResource, )
private:
    std::atomic<uint32> refCounter;
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

    /* ReferenceCountPtr implementation */
    void addRef();
    void removeRef();
    uint32 refCount() const;
    /* GraphicsResource overrides */
    String getResourceName() const override;
    void setResourceName(const String& name) override;
    /* overrides ends */
};
using MemoryResourceRef = ReferenceCountPtr<MemoryResource>;

class ENGINERENDERER_EXPORT BufferResource : public MemoryResource
{
    DECLARE_GRAPHICS_RESOURCE(BufferResource, , MemoryResource, )

public:
    void setAsStagingResource(bool isStaging) { bIsStagingResource = isStaging; }

    // Valid value only in case of texel buffer, ordinary buffer gives EPixelDataFormat::Undefined
    EPixelDataFormat::Type texelFormat() const { return dataFormat; }
    virtual void setTexelFormat(EPixelDataFormat::Type format) {}
    virtual uint32 bufferStride() const { return 0; }
    virtual void setBufferStride(uint32 newStride) {}
    virtual uint32 bufferCount() const { return 0; }
    virtual void setBufferCount(uint32 newCount) {}
};
using BufferResourceRef = ReferenceCountPtr<BufferResource>;

struct ImageResourceCreateInfo
{
    EPixelDataFormat::Type imageFormat;
    Size3D dimensions{ 256,256,1 };
    uint32 numOfMips{ 0 };
    uint32 layerCount{ 1 };
};

class ENGINERENDERER_EXPORT ImageResource : public MemoryResource
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

    uint32 mipCountFromDim();
public:
    ImageResource(ImageResourceCreateInfo createInfo);

    void setLayerCount(uint32 count);

    void setSampleCounts(EPixelSampleCount::Type samples);
    void setNumOfMips(uint32 mipCount);

    void setShaderUsage(uint32 usage);
    void setImageSize(const Size3D& imageSize);

    FORCE_INLINE uint32 getLayerCount() const { return layerCount; }
    FORCE_INLINE uint32 getNumOfMips() const { return numOfMips; };
    FORCE_INLINE const Size3D& getImageSize() const { return dimensions; }
    FORCE_INLINE EPixelDataFormat::Type imageFormat() const { return dataFormat; }
    FORCE_INLINE EPixelSampleCount::Type sampleCount() const { return sampleCounts; }
    FORCE_INLINE bool isShaderRead() const { return (shaderUsage & EImageShaderUsage::Sampling) > 0; }
    FORCE_INLINE bool isShaderWrite() const { return (shaderUsage & EImageShaderUsage::Writing) > 0; }
};
using ImageResourceRef = ReferenceCountPtr<ImageResource>;

struct CopyBufferInfo
{
    uint64 srcOffset;
    uint64 dstOffset;
    uint32 copySize;
};
// Single copy from src to dst
struct BatchCopyBufferInfo
{
    BufferResourceRef src;
    BufferResourceRef dst;
    CopyBufferInfo copyInfo;
};

// Single copy struct, Used to copy cpu visible data to a gpu(can be cpu visible) buffer
struct BatchCopyBufferData
{
    BufferResourceRef dst;
    uint32 dstOffset;
    const void* dataToCopy;
    uint32 size;
};

struct CopyPixelsToImageInfo
{
    // Offset and extent for MIP base rest will be calculated automatically
    Size3D srcOffset;
    Size3D dstOffset;
    Size3D extent;

    ImageSubresource subres;

    bool bGenerateMips;
    // Filtering to be used to generate MIPs
    ESamplerFiltering::Type mipFiltering;
};

struct CopyImageInfo
{
    // Offset and extent for MIP base rest will be calculated automatically
    Size3D offset{ 0 };
    Size3D extent;

    ImageSubresource subres;

    FORCE_INLINE bool isCopyCompatible(const CopyImageInfo& rhs) const
    {
        return extent == rhs.extent && subres == rhs.subres;
    }
};

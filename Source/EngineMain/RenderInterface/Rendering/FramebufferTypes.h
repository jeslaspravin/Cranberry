#pragma once
#include "../CoreGraphicsTypes.h"
#include "../../Core/Types/HashTypes.h"

#include <vector>

namespace ERenderPassFormat
{
    enum Type
    {
        Generic,
        Multibuffers,
        Depth
    };

    String toString(ERenderPassFormat::Type renderpassFormat);

#define FOR_EACH_RENDERPASS_FORMAT(OpMacro) \
    OpMacro(Generic),                       \
    OpMacro(Multibuffers),                  \
    OpMacro(Depth)
}


//////////////////////////////////////////////////////////////////////////
// Framebuffer types
//////////////////////////////////////////////////////////////////////////

class ImageResource;

struct FramebufferFormat
{
    std::vector<EPixelDataFormat::Type> attachments;
    ERenderPassFormat::Type rpFormat;

    explicit FramebufferFormat(std::vector<EPixelDataFormat::Type>&& frameBuffers, ERenderPassFormat::Type renderpassFormat);
    explicit FramebufferFormat(ERenderPassFormat::Type renderpassFormat) : rpFormat(renderpassFormat){}

    bool operator==(const FramebufferFormat& otherFormat) const;
    bool operator<(const FramebufferFormat& otherFormat) const;
};

template <>
struct std::hash<FramebufferFormat> 
{
    _NODISCARD size_t operator()(const FramebufferFormat& keyval) const noexcept 
    {
        // If generic then keying is based on formats
        if (keyval.rpFormat == ERenderPassFormat::Generic)
        {
            size_t hashVal = HashUtility::hash(keyval.attachments.size());
            for (const EPixelDataFormat::Type& format : keyval.attachments)
            {
                HashUtility::hashCombine(hashVal, format);
            }
            return hashVal;
        }        
        return HashUtility::hash(keyval.rpFormat);
    }
};

struct Framebuffer
{
    std::vector<ImageResource*> textures;
    // If true then all color attachments will definitely have a resolve to it and it will be next to each color attachment
    bool bHasResolves = false;

    virtual ~Framebuffer() = default;
};


/*
* Complying with our assumptions on how complex a render pass can be see VulkanFrameBuffer.cpp
*/
struct GenericRenderPassProperties
{
    FramebufferFormat renderpassAttachmentFormat;
    EPixelSampleCount::Type multisampleCount;
    // if all RT used is using same read write textures?
    bool bOneRtPerFormat;
    
    GenericRenderPassProperties() : renderpassAttachmentFormat(ERenderPassFormat::Generic) {}

    bool operator==(const GenericRenderPassProperties& otherProperties) const;
};

template <>
struct std::hash<GenericRenderPassProperties>
{
    _NODISCARD size_t operator()(const GenericRenderPassProperties& keyval) const noexcept
    {
        size_t hashVal = HashUtility::hash(keyval.renderpassAttachmentFormat);
        HashUtility::hashCombine(hashVal, keyval.multisampleCount);
        HashUtility::hashCombine(hashVal, keyval.bOneRtPerFormat);
        return hashVal;
    }
};

struct RenderPassAdditionalProps
{
    EAttachmentOp::LoadOp colorAttachmentLoadOp = EAttachmentOp::LoadOp::Clear;
    EAttachmentOp::StoreOp colorAttachmentStoreOp = EAttachmentOp::StoreOp::Store;

    EAttachmentOp::LoadOp depthLoadOp = EAttachmentOp::LoadOp::Clear;
    EAttachmentOp::StoreOp depthStoreOp = EAttachmentOp::StoreOp::Store;

    EAttachmentOp::LoadOp stencilLoadOp = EAttachmentOp::LoadOp::Clear;
    EAttachmentOp::StoreOp stencilStoreOp = EAttachmentOp::StoreOp::Store;

    // If attachment initial layout can be undefined
    bool bAllowUndefinedLayout = true;
    // If attachments be used as present source
    bool bUsedAsPresentSource = false;

    constexpr bool operator==(const RenderPassAdditionalProps& otherProps) const
    {
        return colorAttachmentLoadOp == otherProps.colorAttachmentLoadOp && colorAttachmentStoreOp == otherProps.colorAttachmentStoreOp
            && depthLoadOp == otherProps.depthLoadOp && depthStoreOp == otherProps.depthStoreOp
            && stencilLoadOp == otherProps.stencilLoadOp && stencilStoreOp == otherProps.stencilStoreOp
            && bAllowUndefinedLayout == otherProps.bAllowUndefinedLayout && bUsedAsPresentSource == otherProps.bUsedAsPresentSource;
    }
};
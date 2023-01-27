/*!
 * \file FramebufferTypes.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once
#include <vector>

#include "EngineRendererExports.h"
#include "RenderInterface/CoreGraphicsTypes.h"
#include "RenderInterface/Resources/MemoryResources.h"
#include "Types/HashTypes.h"

//////////////////////////////////////////////////////////////////////////
// Framebuffer types
//////////////////////////////////////////////////////////////////////////

struct ENGINERENDERER_EXPORT FramebufferFormat
{
public:
    using AttachmentsFormatList = std::vector<EPixelDataFormat::Type>;
    // One format per RT and resolve pair
    AttachmentsFormatList attachments;
    ERenderPassFormat::Type rpFormat;

    explicit FramebufferFormat(AttachmentsFormatList &&frameBuffers, ERenderPassFormat::Type renderpassFormat)
        : attachments(std::move(frameBuffers))
        , rpFormat(renderpassFormat)
    {}
    explicit FramebufferFormat(const AttachmentsFormatList &frameBuffers, ERenderPassFormat::Type renderpassFormat)
        : attachments(frameBuffers)
        , rpFormat(renderpassFormat)
    {}
    explicit FramebufferFormat(ERenderPassFormat::Type renderpassFormat)
        : rpFormat(renderpassFormat)
    {}

    bool operator== (const FramebufferFormat &otherFormat) const;
    bool operator< (const FramebufferFormat &otherFormat) const;
};

template <>
struct ENGINERENDERER_EXPORT std::hash<FramebufferFormat>
{
    _NODISCARD size_t operator() (const FramebufferFormat &keyval) const noexcept
    {
        // If generic then keying is based on formats
        if (keyval.rpFormat == ERenderPassFormat::Generic)
        {
            size_t hashVal = HashUtility::hash(keyval.attachments.size());
            for (const EPixelDataFormat::Type &format : keyval.attachments)
            {
                HashUtility::hashCombine(hashVal, format);
            }
            return hashVal;
        }
        return HashUtility::hash(keyval.rpFormat);
    }
};

struct ENGINERENDERER_EXPORT Framebuffer
{
    std::vector<ImageResourceRef> textures;
    // If true then all color attachments will definitely have a resolve to it and it will be next to
    // each color attachment
    bool bHasResolves = false;

    virtual ~Framebuffer() = default;
};

/*
 * Complying with our assumptions on how complex a render pass can be see VulkanFrameBuffer.cpp
 */
struct ENGINERENDERER_EXPORT GenericRenderPassProperties
{
    FramebufferFormat renderpassAttachmentFormat;
    EPixelSampleCount::Type multisampleCount;
    // if all RT used is using same read write textures?
    bool bOneRtPerFormat;

    GenericRenderPassProperties()
        : renderpassAttachmentFormat(ERenderPassFormat::Generic)
        , multisampleCount(EPixelSampleCount::SampleCount1)
        , bOneRtPerFormat(true)
    {}

    bool operator== (const GenericRenderPassProperties &otherProperties) const;
};

template <>
struct ENGINERENDERER_EXPORT std::hash<GenericRenderPassProperties>
{
    _NODISCARD size_t operator() (const GenericRenderPassProperties &keyval) const noexcept
    {
        size_t hashVal = HashUtility::hash(keyval.renderpassAttachmentFormat);
        HashUtility::hashCombine(hashVal, keyval.multisampleCount);
        HashUtility::hashCombine(hashVal, keyval.bOneRtPerFormat);
        return hashVal;
    }
};

struct ENGINERENDERER_EXPORT RenderPassAdditionalProps
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

    constexpr bool operator== (const RenderPassAdditionalProps &otherProps) const
    {
        return colorAttachmentLoadOp == otherProps.colorAttachmentLoadOp && colorAttachmentStoreOp == otherProps.colorAttachmentStoreOp
               && depthLoadOp == otherProps.depthLoadOp && depthStoreOp == otherProps.depthStoreOp && stencilLoadOp == otherProps.stencilLoadOp
               && stencilStoreOp == otherProps.stencilStoreOp && bAllowUndefinedLayout == otherProps.bAllowUndefinedLayout
               && bUsedAsPresentSource == otherProps.bUsedAsPresentSource;
    }
};
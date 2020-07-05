#pragma once
#include "../CoreGraphicsTypes.h"
#include "../../Core/Types/HashTypes.h"

#include <vector>

namespace ERenderpassFormat
{
    enum Type
    {
        Generic,
        Multibuffers,
        Depth
    };

    String toString(ERenderpassFormat::Type renderpassFormat)
    {
        switch (renderpassFormat)
        {
        case ERenderpassFormat::Generic:
            return "Generic";
        case ERenderpassFormat::Multibuffers:
            return "Multibuffer";
        case ERenderpassFormat::Depth:
            return "Depth";
        }
        return "";
    }

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
    ERenderpassFormat::Type rpFormat;

    explicit FramebufferFormat(std::vector<EPixelDataFormat::Type>&& frameBuffers, ERenderpassFormat::Type renderpassFormat);
    explicit FramebufferFormat(ERenderpassFormat::Type renderpassFormat) : rpFormat(renderpassFormat){}

    bool operator==(const FramebufferFormat& otherFormat) const;
    bool operator<(const FramebufferFormat& otherFormat) const;
};

template <>
struct std::hash<FramebufferFormat> 
{
    _NODISCARD size_t operator()(const FramebufferFormat& keyval) const noexcept 
    {
        // If generic then keying is based on formats
        if (keyval.rpFormat == ERenderpassFormat::Generic)
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
struct GenericRenderpassProperties
{
    FramebufferFormat renderpassAttachmentFormat;
    EPixelSampleCount::Type multisampleCount;
    // if all RT used is using same read write textures?
    bool bOneRtPerFormat;
    
    GenericRenderpassProperties() : renderpassAttachmentFormat(ERenderpassFormat::Generic) {}

    bool operator==(const GenericRenderpassProperties& otherProperties) const;
};

bool GenericRenderpassProperties::operator==(const GenericRenderpassProperties& otherProperties) const
{
    return renderpassAttachmentFormat == otherProperties.renderpassAttachmentFormat && multisampleCount == otherProperties.multisampleCount 
        && bOneRtPerFormat == otherProperties.bOneRtPerFormat;
}

template <>
struct std::hash<GenericRenderpassProperties>
{
    _NODISCARD size_t operator()(const GenericRenderpassProperties& keyval) const noexcept
    {
        size_t hashVal = HashUtility::hash(keyval.renderpassAttachmentFormat);
        HashUtility::hashCombine(hashVal, keyval.multisampleCount);
        HashUtility::hashCombine(hashVal, keyval.bOneRtPerFormat);
        return hashVal;
    }
};
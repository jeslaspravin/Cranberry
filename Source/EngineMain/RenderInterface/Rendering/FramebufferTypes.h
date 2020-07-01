#pragma once
#include "../CoreGraphicsTypes.h"
#include "../../Core/Types/HashTypes.h"

#include <vector>

namespace ERenderpassFormat
{
    enum Type
    {
        Multibuffers,
        Depth
    };

    String toString(ERenderpassFormat::Type renderpassFormat)
    {
        switch (renderpassFormat)
        {
        case ERenderpassFormat::Multibuffers:
            return "Multibuffer";
        case ERenderpassFormat::Depth:
            return "Depth";
        }
        return "";
    }

#define FOR_EACH_RENDERPASS_FORMAT(OpMacro) \
    OpMacro(Multibuffers)                   \
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
        //size_t hashVal = std::hash<size_t>{}(keyval.attachments.size());
        //for (const EPixelDataFormat::Type& format : keyval.attachments)
        //{
        //    HashUtility::hashCombine(hashVal, format);
        //}
        //return hashVal;
        return std::hash<decltype(keyval.rpFormat)>{}(keyval.rpFormat);
    }
};

struct Framebuffer
{
    std::vector<ImageResource*> textures;
    // If true then all color attachments will definitely have a resolve to it and it will be next to each color attachment
    bool bHasResolves = false;

    virtual ~Framebuffer() = default;
};
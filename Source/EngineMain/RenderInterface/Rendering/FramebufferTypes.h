#pragma once
#include "../CoreGraphicsTypes.h"
#include "../../Core/Types/HashTypes.h"

#include <vector>


//////////////////////////////////////////////////////////////////////////
// Framebuffer types
//////////////////////////////////////////////////////////////////////////

class ImageResource;

struct FramebufferFormat
{
    std::vector<EPixelDataFormat::Type> attachments;
    FramebufferFormat(std::vector<EPixelDataFormat::Type>&& frameBuffers);

    bool operator==(const FramebufferFormat& otherFormat) const;
};
template <>
struct std::hash<FramebufferFormat> {

    _NODISCARD size_t operator()(const FramebufferFormat& keyval) const noexcept {
        size_t hashVal = std::hash<size_t>{}(keyval.attachments.size());
        for (const EPixelDataFormat::Type& format : keyval.attachments)
        {
            HashUtility::hashCombine(hashVal, format);
        }
        return hashVal;
    }
};

struct Framebuffer
{
    std::vector<ImageResource*> textures;
    // If true then all color attachments will definitely have a resolve to it and it will be next to each color attachment
    bool bHasResolves = false;

    virtual ~Framebuffer() = default;
};
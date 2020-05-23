#pragma once
#include "../RenderInterface/CoreGraphicsTypes.h"
#include "../Core/Math/CoreMathTypedefs.h"
#include "../Core/Types/HashTypes.h"

#include <vector>
#include <unordered_map>

class RenderTargetTexture;

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
        for (auto format : keyval.attachments)
        {
            HashUtility::hashCombine(hashVal, format);
        }
        return hashVal;
    }
};

struct Framebuffer
{
    std::vector<RenderTargetTexture*> textures;

    virtual ~Framebuffer() = default;
    class ImageResource* getImageResource(const RenderTargetTexture * rtTexture) const;
};

class GBuffers
{
private:
    // Frame buffer format to frame buffers swapchain count times
    static std::unordered_map<FramebufferFormat, std::vector<Framebuffer*>> gBuffers;

    static Framebuffer* createFbInternal();
    static void initializeInternal(Framebuffer* fb);
    static void onSampleCountChanged(uint32 oldValue, uint32 newValue);
    static void onResize(Size2D oldSize, Size2D newSize);
public:
    static void initialize();
    static void destroy();
};
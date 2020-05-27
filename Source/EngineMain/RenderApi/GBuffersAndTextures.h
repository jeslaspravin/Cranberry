#pragma once
#include "../RenderInterface/CoreGraphicsTypes.h"
#include "../Core/Math/CoreMathTypedefs.h"
#include "../Core/Types/HashTypes.h"

#include <vector>
#include <unordered_map>

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

    virtual ~Framebuffer() = default;
};

class GBuffers
{
private:
    // Frame buffer format to frame buffers swapchain count times
    static std::unordered_map<FramebufferFormat, std::vector<Framebuffer*>> gBuffers;
    static std::vector<Framebuffer*> swapchainFbs;

    static Framebuffer* createFbInternal();
    static void initializeInternal(Framebuffer* fb, const Size2D& frameSize);
    static void initializeSwapchainFb(Framebuffer* fb, const class GenericWindowCanvas* canvas, const Size2D& frameSize, uint32 swapchainIdx);
    static void onSampleCountChanged(uint32 oldValue, uint32 newValue);
public:
    static void initialize();
    static void destroy();

    static Framebuffer* getFramebuffer(const FramebufferFormat& framebufferFormat, uint32 frameIdx);
    static Framebuffer* getSwapchainFramebuffer(uint32 frameIdx);

    static void onScreenResized(Size2D newSize);
    static void onSurfaceResized(Size2D newSize);
};
#pragma once
#include "../RenderInterface/Rendering/FramebufferTypes.h"
#include "../Core/Math/CoreMathTypedefs.h"

#include <vector>
#include <unordered_map>

struct FramebufferWrapper
{
    std::vector<class GBufferRenderTexture*> rtTextures;
    Framebuffer* framebuffer;
};

class GBuffers
{
private:
    // Frame buffer format to frame buffers swapchain count times
    static std::unordered_map<FramebufferFormat, std::vector<FramebufferWrapper>> gBuffers;
    static std::vector<Framebuffer*> swapchainFbs;

    static Framebuffer* createFbInternal();
    // Destroy if any already existing raw framebuffer resource
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
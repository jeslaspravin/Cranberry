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

    static void initializeSwapchainFb(Framebuffer* fb, const class GenericWindowCanvas* canvas, const Size2D& frameSize, uint32 swapchainIdx);
    static void onSampleCountChanged(uint32 oldValue, uint32 newValue);
public:
    static void initialize();
    static void destroy();

    /**
    * GBuffers::getFramebuffer - Gets framebuffer and framebuffer format for a renderpass format passed in with framebuffer format
    *
    * Access:    public static 
    *
    * @param FramebufferFormat & framebufferFormat - Format struct that contains renderpass format type and gets filled back with framebuffer format as response
    * @param uint32 frameIdx - Current frame's swapchain acquired index
    *
    * @return Framebuffer* - framebuffer wrapper object ptr
    */
    static Framebuffer* getFramebuffer(FramebufferFormat& framebufferFormat, uint32 frameIdx);
    static Framebuffer* getFramebuffer(ERenderpassFormat::Type renderpassFormat, uint32 frameIdx);
    static Framebuffer* getSwapchainFramebuffer(uint32 frameIdx);

    static void onScreenResized(Size2D newSize);
    static void onSurfaceResized(Size2D newSize);

    static Framebuffer* createFbInstance();
    // Destroy if any already existing raw framebuffer resource
    static void initializeFb(Framebuffer* fb, const Size2D& frameSize);
};
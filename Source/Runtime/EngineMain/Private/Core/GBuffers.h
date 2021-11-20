#pragma once
#include "RenderApi/GBuffersAndTextures.h"


struct GbufferWrapper
{
    std::vector<class GBufferRenderTexture*> rtTextures;
};

class GBuffers
{
private:
    // Frame buffer format to frame buffers swapchain count times
    static std::unordered_map<FramebufferFormat, std::vector<GbufferWrapper>>& gBuffers();

private:

    static void onSampleCountChanged(uint32 oldValue, uint32 newValue);
public:
    static void initialize(int32 swapchainCount);
    static void destroy();

    static void onScreenResized(Size2D newSize);

    static std::vector<class IRenderTargetTexture*> getGbufferRts(ERenderPassFormat::Type renderpassFormat, uint32 frameIdx);
    static std::vector<ImageResourceRef> getGbufferAttachments(ERenderPassFormat::Type renderpassFormat, uint32 frameIdx);
};
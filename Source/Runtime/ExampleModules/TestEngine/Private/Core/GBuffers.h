/*!
 * \file GBuffers.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once
#include "RenderApi/GBuffersAndTextures.h"

class IRenderTargetTexture;
class GBufferRenderTexture;

struct GbufferWrapper
{
    std::vector<GBufferRenderTexture *> rtTextures;
};

class GBuffers
{
private:
    // Frame buffer format to frame buffers swapchain count times
    static std::unordered_map<FramebufferFormat, std::vector<GbufferWrapper>> &gBuffers();

private:
    static void onSampleCountChanged(uint32 oldValue, uint32 newValue);

public:
    static void initialize(uint32 swapchainCount);
    static void destroy();

    static void onScreenResized(UInt2 newSize);

    static std::vector<const IRenderTargetTexture *> getGbufferRts(ERenderPassFormat::Type renderpassFormat, uint32 frameIdx);
    static std::vector<ImageResourceRef> getGbufferAttachments(ERenderPassFormat::Type renderpassFormat, uint32 frameIdx);
};
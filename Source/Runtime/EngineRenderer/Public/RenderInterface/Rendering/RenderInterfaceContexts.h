/*!
 * \file RenderInterfaceContexts.h
 * \brief Contexts used by RenderInterface itself, Differs from RenderApi/Rendering/RenderingContexts.h by this point only
 *
 * \author Jeslas
 * \date June 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "EngineRendererExports.h"
#include "RenderInterface/Resources/MemoryResources.h"
#include "RenderInterface/Resources/GenericWindowCanvas.h"
#include "RenderApi/VertexData.h"

struct Framebuffer;
class PipelineBase;

class ENGINERENDERER_EXPORT LocalPipelineContext
{
private:
    friend class GlobalRenderingContextBase;

    const Framebuffer *framebuffer = nullptr;
    const PipelineBase *pipelineUsed = nullptr;

public:
    // Will be filled by RenderManager
    std::vector<ImageResourceRef> frameAttachments;
    ERenderPassFormat::Type renderpassFormat;

    // Used if Generic render pass and swapchain is going to be used as a FrameBuffer attachment
    uint32 swapchainIdx;
    WindowCanvasRef windowCanvas;

    // Used only for predefined render pass formats(renderpassFormat != ERenderPassFormat::Generic)
    EVertexType::Type forVertexType;

    NameString materialName;

    const Framebuffer *getFb() const { return framebuffer; }
    const PipelineBase *getPipeline() const { return pipelineUsed; }

    // Reset all Reference resources held by this context
    FORCE_INLINE void reset()
    {
        windowCanvas.reset();
        frameAttachments.clear();
    }
};
/*!
 * \file GBuffersAndTextures.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once
#include "EngineRendererExports.h"
#include "Math/CoreMathTypedefs.h"
#include "RenderInterface/Resources/Samplers/SamplerInterface.h"
#include "RenderInterface/Rendering/FramebufferTypes.h"

#include <unordered_map>
#include <vector>

class IGraphicsInstance;
class IRenderCommandList;
class GraphicsHelperAPI;

class ENGINERENDERER_EXPORT GlobalBuffers
{
private:
    // Attachment formats for GBuffers render pass types
    static std::unordered_map<ERenderPassFormat::Type, FramebufferFormat::AttachmentsFormatList> GBUFFERS_ATTACHMENT_FORMATS;

    static ImageResourceRef dummyBlackTexture;
    static ImageResourceRef dummyWhiteTexture;
    static ImageResourceRef dummyCubeTexture;
    static ImageResourceRef dummyNormalTexture;
    static ImageResourceRef dummyDepthTexture;

    static ImageResourceRef integratedBRDF;

    static BufferResourceRef quadTriVertsBuffer;
    static std::pair<BufferResourceRef, BufferResourceRef> quadRectVertsInds;
    static std::pair<BufferResourceRef, BufferResourceRef> lineGizmoVertsInds;

    static SamplerRef nearestFiltering;
    static SamplerRef linearFiltering;
    static SamplerRef depthFiltering;
    static SamplerRef shadowFiltering;

public:
    /**
     * Both must be called from render thread
     */
    static void initialize();
    static void destroy();

    static GenericRenderPassProperties getFramebufferRenderpassProps(ERenderPassFormat::Type renderpassFormat);
    static const FramebufferFormat::AttachmentsFormatList &getGBufferAttachmentFormat(ERenderPassFormat::Type renderpassFormat);

    static ImageResourceRef dummyWhite2D() { return dummyWhiteTexture; }
    static ImageResourceRef dummyBlack2D() { return dummyBlackTexture; }
    static ImageResourceRef dummyCube() { return dummyCubeTexture; }
    static ImageResourceRef dummyNormal() { return dummyNormalTexture; }
    static ImageResourceRef dummyDepth() { return dummyDepthTexture; }
    static ImageResourceRef integratedBrdfLUT() { return integratedBRDF; }

    static SamplerRef nearestSampler() { return nearestFiltering; }
    static SamplerRef linearSampler() { return linearFiltering; }
    static SamplerRef depthSampler() { return depthFiltering; }
    static SamplerRef shadowSampler() { return shadowFiltering; }

    static BufferResourceRef getQuadTriVertexBuffer() { return quadTriVertsBuffer; }
    static std::pair<BufferResourceRef, BufferResourceRef> getQuadRectVertexIndexBuffers() { return quadRectVertsInds; }
    static std::pair<BufferResourceRef, BufferResourceRef> getLineGizmoVertexIndexBuffers() { return lineGizmoVertsInds; }

private:
    static void createTextureCubes(IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper);
    static void destroyTextureCubes();

    static void createTexture2Ds(IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper);
    // Generates using shaders or some other pipeline based techniqs
    static void generateTexture2Ds(IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper);
    static void destroyTexture2Ds();

    static void createVertIndBuffers(IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper);
    static void destroyVertIndBuffers();

    static void createSamplers(IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper);
    static void destroySamplers();
};
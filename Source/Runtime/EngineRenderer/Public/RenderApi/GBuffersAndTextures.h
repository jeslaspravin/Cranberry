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
#include "RenderInterface/Rendering/FramebufferTypes.h"
#include "Math/CoreMathTypedefs.h"
#include "EngineRendererExports.h"

#include <vector>
#include <unordered_map>

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

    static ImageResourceRef integratedBRDF;

    static BufferResourceRef quadTriVerts;
    static std::pair<BufferResourceRef, BufferResourceRef> quadRectVertsInds;
    static std::pair<BufferResourceRef, BufferResourceRef> lineGizmoVertxInds;

    static void createTextureCubes(IRenderCommandList* cmdList, IGraphicsInstance* graphicsInstance, const GraphicsHelperAPI* graphicsHelper);
    static void destroyTextureCubes();

    static void createTexture2Ds(IRenderCommandList* cmdList, IGraphicsInstance* graphicsInstance, const GraphicsHelperAPI* graphicsHelper);
    // Generates using shaders or some other pipeline based technics
    static void generateTexture2Ds();
    static void destroyTexture2Ds();

    static void createVertIndBuffers(IRenderCommandList* cmdList, IGraphicsInstance* graphicsInstance, const GraphicsHelperAPI* graphicsHelper);
    static void destroyVertIndBuffers();
public:
    static void initialize();
    static void destroy();

    static GenericRenderPassProperties getFramebufferRenderpassProps(ERenderPassFormat::Type renderpassFormat);
    static const FramebufferFormat::AttachmentsFormatList& getGBufferAttachmentFormat(ERenderPassFormat::Type renderpassFormat);

    static ImageResourceRef dummyWhite2D() { return dummyWhiteTexture; }
    static ImageResourceRef dummyBlack2D() { return dummyBlackTexture; }
    static ImageResourceRef dummyCube() { return dummyCubeTexture; }
    static ImageResourceRef dummyNormal() { return dummyNormalTexture; }
    static ImageResourceRef integratedBrdfLUT() { return integratedBRDF; }

    static const BufferResourceRef getQuadTriVertexBuffer() { return quadTriVerts; }
    static std::pair<const BufferResourceRef, const BufferResourceRef> getQuadRectVertexIndexBuffers() { return quadRectVertsInds; }
    static std::pair<const BufferResourceRef, const BufferResourceRef> getLineGizmoVertexIndexBuffers() { return lineGizmoVertxInds; }
};
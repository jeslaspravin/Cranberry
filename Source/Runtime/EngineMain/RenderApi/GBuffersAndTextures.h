#pragma once
#include "../RenderInterface/Rendering/FramebufferTypes.h"
#include "../Core/Math/CoreMathTypedefs.h"

#include <vector>
#include <unordered_map>

class TextureBase;
class BufferResource;
class IGraphicsInstance;
class IRenderCommandList;

struct FramebufferWrapper
{
    std::vector<class GBufferRenderTexture*> rtTextures;
    Framebuffer* framebuffer;
};

class GlobalBuffers
{
private:
    // Attachment formats for GBuffers render pass types
    static std::unordered_map<ERenderPassFormat::Type, FramebufferFormat::AttachmentsFormatList> GBUFFERS_ATTACHMENT_FORMATS;
    // Frame buffer format to frame buffers swapchain count times
    static std::unordered_map<FramebufferFormat, std::vector<FramebufferWrapper>> gBuffers;
    static std::vector<Framebuffer*> swapchainFbs;

    static TextureBase* dummyBlackTexture;
    static TextureBase* dummyWhiteTexture;
    static TextureBase* dummyCubeTexture;
    static TextureBase* dummyNormalTexture;

    static TextureBase* integratedBRDF;

    static BufferResource* quadTriVerts;
    static std::pair<BufferResource*, BufferResource*> quadRectVertsInds;
    static std::pair<BufferResource*, BufferResource*> lineGizmoVertxInds;

    static void initializeSwapchainFb(Framebuffer* fb, const class GenericWindowCanvas* canvas, const Size2D& frameSize, uint32 swapchainIdx);
    static void onSampleCountChanged(uint32 oldValue, uint32 newValue);

    static void createTextureCubes();
    static void destroyTextureCubes();

    static void createTexture2Ds();
    // Generates using shaders or some other pipeline based technics
    static void generateTexture2Ds();
    static void destroyTexture2Ds();

    static void createVertIndBuffers(IRenderCommandList* cmdList, IGraphicsInstance* graphicsInstance);
    static void destroyVertIndBuffers(IRenderCommandList* cmdList, IGraphicsInstance* graphicsInstance);
public:
    static void initialize();
    static void destroy();

    /**
    * GlobalBuffers::getFramebuffer - Gets framebuffer and framebuffer format for a renderpass format passed in with framebuffer format
    *
    * Access:    public static 
    *
    * @param FramebufferFormat & framebufferFormat - Format struct that contains renderpass format type and gets filled back with framebuffer format as response
    * @param uint32 frameIdx - Current frame's swapchain acquired index
    *
    * @return Framebuffer* - framebuffer wrapper object ptr
    */
    static Framebuffer* getFramebuffer(FramebufferFormat& framebufferFormat, uint32 frameIdx);
    static Framebuffer* getFramebuffer(ERenderPassFormat::Type renderpassFormat, uint32 frameIdx);
    static std::vector<class RenderTargetTexture*> getFramebufferRts(ERenderPassFormat::Type renderpassFormat, uint32 frameIdx);
    static GenericRenderPassProperties getFramebufferRenderpassProps(ERenderPassFormat::Type renderpassFormat);
    static Framebuffer* getSwapchainFramebuffer(uint32 frameIdx);

    static TextureBase* dummyWhite2D() { return dummyWhiteTexture; }
    static TextureBase* dummyBlack2D() { return dummyBlackTexture; }
    static TextureBase* dummyCube() { return dummyCubeTexture; }
    static TextureBase* dummyNormal() { return dummyNormalTexture; }
    static TextureBase* integratedBrdfLUT() { return integratedBRDF; }

    static void onScreenResized(Size2D newSize);
    static void onSurfaceUpdated();

    static Framebuffer* createFbInstance();
    static void destroyFbInstance(const Framebuffer* fb);
    // Destroy if any already existing raw framebuffer resource
    static void initializeFb(Framebuffer* fb, const Size2D& frameSize);

    static const BufferResource* getQuadTriVertexBuffer() { return quadTriVerts; }
    static std::pair<const BufferResource*, const BufferResource*> getQuadRectVertexIndexBuffers() { return quadRectVertsInds; }
    static std::pair<const BufferResource*, const BufferResource*> getLineGizmoVertexIndexBuffers() { return lineGizmoVertxInds; }
};
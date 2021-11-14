
#include <array>

#include "RenderApi/VertexData.h"
#include "Types/Platform/PlatformAssertionErrors.h"
#include "Math/Vector2D.h"
#include "Math/Vector3D.h"
#include "Math/Vector4D.h"
#include "ShaderDataTypes.h"
#include "RenderApi/GBuffersAndTextures.h"
#include "RenderInterface/Rendering/IRenderCommandList.h"
#include "IRenderInterfaceModule.h"
#include "RenderInterface/GraphicsHelper.h"

BEGIN_VERTEX_DEFINITION(StaticMeshVertex, EShaderInputFrequency::PerVertex)
ADD_VERTEX_FIELD(position)
ADD_VERTEX_FIELD(normal)
ADD_VERTEX_FIELD(tangent)
END_VERTEX_DEFINITION();

// Just for using vertex info to fill all pipeline input information from reflection, Real data will be plain VectorND
struct VertexSimple2D
{
    Vector2D position;
};

struct VertexUI
{
    Vector2D position;
    Vector2D uv;
    uint32 color;
};

struct VertexSimple3D
{
    Vector3D position;
};

struct VertexSimple3DColor
{
    Vector3D position;
    uint32 color;
};

struct VertexInstancedSimple3DColor
{
    uint32 color;
    Vector3D x;
    Vector3D y;
    Vector3D translation;
};

BEGIN_VERTEX_DEFINITION(VertexSimple2D, EShaderInputFrequency::PerVertex)
ADD_VERTEX_FIELD(position)
END_VERTEX_DEFINITION();

BEGIN_VERTEX_DEFINITION(VertexUI, EShaderInputFrequency::PerVertex)
ADD_VERTEX_FIELD(position)
ADD_VERTEX_FIELD(uv)
ADD_VERTEX_FIELD_AND_FORMAT(color, EShaderInputAttribFormat::UInt4Norm)
END_VERTEX_DEFINITION();

BEGIN_VERTEX_DEFINITION(VertexSimple3D, EShaderInputFrequency::PerVertex)
ADD_VERTEX_FIELD(position)
END_VERTEX_DEFINITION();

BEGIN_VERTEX_DEFINITION(VertexSimple3DColor, EShaderInputFrequency::PerVertex)
ADD_VERTEX_FIELD(position)
ADD_VERTEX_FIELD_AND_FORMAT(color, EShaderInputAttribFormat::UInt4Norm)
END_VERTEX_DEFINITION();

BEGIN_VERTEX_DEFINITION(VertexInstancedSimple3DColor, EShaderInputFrequency::PerInstance)
ADD_VERTEX_FIELD_AND_FORMAT(color, EShaderInputAttribFormat::UInt4Norm)
ADD_VERTEX_FIELD(x)
ADD_VERTEX_FIELD(y)
ADD_VERTEX_FIELD(translation)
END_VERTEX_DEFINITION();

namespace EVertexType
{
    VertexSimple3DVertexParamInfo SIMPLE3D_PARAM_INFO;

    template<>
    const std::vector<ShaderVertexParamInfo*>& vertexParamInfo<Simple2>()
    {
        static VertexSimple2DVertexParamInfo STATIC_VERTEX_PARAM_INFO;
        static std::vector<ShaderVertexParamInfo*> VERTEX_PARAMS{ &STATIC_VERTEX_PARAM_INFO };
        return VERTEX_PARAMS;
    }

    template<>
    const std::vector<ShaderVertexParamInfo*>& vertexParamInfo<UI>()
    {
        static VertexUIVertexParamInfo STATIC_VERTEX_PARAM_INFO;
        static std::vector<ShaderVertexParamInfo*> VERTEX_PARAMS{ &STATIC_VERTEX_PARAM_INFO };
        return VERTEX_PARAMS;
    }

    template<>
    const std::vector<ShaderVertexParamInfo*>& vertexParamInfo<Simple3>()
    {
        static std::vector<ShaderVertexParamInfo*> VERTEX_PARAMS{ &SIMPLE3D_PARAM_INFO };
        return VERTEX_PARAMS;
    }
    template<>
    const std::vector<ShaderVertexParamInfo*>& vertexParamInfo<Simple3DColor>()
    {
        static VertexSimple3DColorVertexParamInfo STATIC_VERTEX_PARAM_INFO;
        static std::vector<ShaderVertexParamInfo*> VERTEX_PARAMS{ &STATIC_VERTEX_PARAM_INFO };
        return VERTEX_PARAMS;
    }
    template<>
    const std::vector<ShaderVertexParamInfo*>& vertexParamInfo<BasicMesh>()
    {
        static std::vector<ShaderVertexParamInfo*> VERTEX_PARAMS;
        debugAssert(!"Not implemented");
        return VERTEX_PARAMS;
    }
    template<>
    const std::vector<ShaderVertexParamInfo*>& vertexParamInfo<StaticMesh>()
    {
        static StaticMeshVertexVertexParamInfo STATIC_VERTEX_PARAM_INFO;
        static std::vector<ShaderVertexParamInfo*> VERTEX_PARAMS{ &STATIC_VERTEX_PARAM_INFO };
        return VERTEX_PARAMS;
    }

    template<>
    const std::vector<ShaderVertexParamInfo*>& vertexParamInfo<InstancedSimple3DColor>()
    {
        static VertexInstancedSimple3DColorVertexParamInfo STATIC_VERTEX_PARAM_INFO;
        static std::vector<ShaderVertexParamInfo*> VERTEX_PARAMS{ &SIMPLE3D_PARAM_INFO, &STATIC_VERTEX_PARAM_INFO };
        return VERTEX_PARAMS;
    }
    template<>
    const std::vector<ShaderVertexParamInfo*>& vertexParamInfo<NoVertex>()
    {
        static std::vector<ShaderVertexParamInfo*> VERTEX_PARAMS;
        return VERTEX_PARAMS;
    }

    const std::vector<ShaderVertexParamInfo*>& vertexParamInfo(Type vertexType)
    {
        switch (vertexType)
        {
        case EVertexType::Simple2:
            return vertexParamInfo<Simple2>();
        case EVertexType::UI:
            return vertexParamInfo<UI>();
        case EVertexType::Simple3:
            return vertexParamInfo<Simple3>();
        case EVertexType::Simple3DColor:
            return vertexParamInfo<Simple3DColor>();
        case EVertexType::StaticMesh:
            return vertexParamInfo<StaticMesh>();
        case EVertexType::InstancedSimple3DColor:
            return vertexParamInfo<InstancedSimple3DColor>();
        case EVertexType::BasicMesh:
            return vertexParamInfo<BasicMesh>();
        case EVertexType::NoVertex:
        default:
            return vertexParamInfo<NoVertex>();
        }
    }

    String toString(Type vertexType)
    {
        switch (vertexType)
        {
        case EVertexType::Simple2:
            return "Simple2d";
        case EVertexType::Simple3:
            return "Simple3d";
            break;
        case EVertexType::Simple3DColor:
            return "Simple3dColor";
            break;
        case EVertexType::BasicMesh:
            return "BasicMesh";
            break;
        case EVertexType::StaticMesh:
            return "StaticMesh";
            break;
        case EVertexType::InstancedSimple3DColor:
            return "InstSimple3dColor";
        case EVertexType::NoVertex:
            return "NoVertex";
        }
        return "";
    }

    template<>
    void vertexSpecConsts<Simple2>(std::map<String, struct SpecializationConstantEntry>& specializationConst)
    {
    }

    template<>
    void vertexSpecConsts<UI>(std::map<String, struct SpecializationConstantEntry>& specializationConst)
    {
    }

    template<>
    void vertexSpecConsts<Simple3>(std::map<String, struct SpecializationConstantEntry>& specializationConst)
    {
    }
    template<>
    void vertexSpecConsts<Simple3DColor>(std::map<String, struct SpecializationConstantEntry>& specializationConst)
    {
    }
    template<>
    void vertexSpecConsts<BasicMesh>(std::map<String, struct SpecializationConstantEntry>& specializationConst)
    {
    }
    template<>
    void vertexSpecConsts<StaticMesh>(std::map<String, struct SpecializationConstantEntry>& specializationConst)
    {
    }
    template<>
    void vertexSpecConsts<InstancedSimple3DColor>(std::map<String, struct SpecializationConstantEntry>& specializationConst)
    {
    }
    template<>
    void vertexSpecConsts<NoVertex>(std::map<String, SpecializationConstantEntry>& specializationConst)
    {}

    void vertexSpecConsts(Type vertexType, std::map<String, SpecializationConstantEntry>& specializationConst)
    {
        switch (vertexType)
        {
        case EVertexType::Simple2:
            return vertexSpecConsts<Simple2>(specializationConst);
        case EVertexType::UI:
            return vertexSpecConsts<UI>(specializationConst);
        case EVertexType::Simple3:
            return vertexSpecConsts<Simple3>(specializationConst);
        case EVertexType::Simple3DColor:
            return vertexSpecConsts<Simple3DColor>(specializationConst);
        case EVertexType::BasicMesh:
            return vertexSpecConsts<BasicMesh>(specializationConst);
        case EVertexType::StaticMesh:
            return vertexSpecConsts<StaticMesh>(specializationConst);
        case EVertexType::InstancedSimple3DColor:
            return vertexSpecConsts<InstancedSimple3DColor>(specializationConst);
        case EVertexType::NoVertex:
        default:
            return vertexSpecConsts<NoVertex>(specializationConst);
        }
    }

}


void GlobalBuffers::destroyVertIndBuffers()
{
    lineGizmoVertxInds.first.reset();
    lineGizmoVertxInds.second.reset();

    quadTriVerts.reset();

    quadRectVertsInds.first.reset();
    quadRectVertsInds.second.reset();
}

void GlobalBuffers::createVertIndBuffers(class IRenderCommandList* cmdList, IGraphicsInstance* graphicsInstance, const GraphicsHelperAPI* graphicsHelper)
{
    const std::array<Vector3D, 3> quadTriVerts = { Vector3D(-1,-1,0),Vector3D(3,-1,0),Vector3D(-1,3,0) };
    // const std::array<uint32, 3> quadTriIndices = { 0,1,2 };// 3 Per tri of quad

    const std::array<Vector3D, 4> quadRectVerts = { Vector3D(-1,-1,0),Vector3D(1,-1,0),Vector3D(-1,1,0), Vector3D(1,1,0) };
    const std::array<uint32, 6> quadRectIndices = { 0, 1, 2, 2, 1, 3 };

    // 0-17(18) for axis arrows 18-29(12) for letters
    std::array<VertexSimple3DColor, 30> gizmoVerts;
    // 0-29(30) for axis arrows 30-45(16) for letters
    std::array<uint32, 46> gizmoIndices;
    const uint32 idxPerAxis = 6;
    const uint32 vertPerAxis = 10;
    for (uint32 axis = 0; axis < 3; ++axis)
    {
        Vector3D axisVector;
        Vector3D otheAxis1;
        Vector3D otheAxis2;
        Color color;
        switch (axis)
        {
        case 0:
        {
            axisVector = Vector3D::FWD;
            otheAxis1 = Vector3D::RIGHT;
            otheAxis2 = Vector3D::UP;
            color = ColorConst::RED;

            // Letter X
            const uint32 startVert = 18;
            const uint32 startIdx = 30;
            gizmoVerts[startVert + 0] = { (axisVector * 120) + (Vector3D::UP * 10) + (Vector3D::RIGHT * 8), color };
            gizmoVerts[startVert + 1] = { (axisVector * 120) - (Vector3D::UP * 10) - (Vector3D::RIGHT * 8), color };
            gizmoIndices[startIdx + 0] = startVert;
            gizmoIndices[startIdx + 1] = startVert + 1;

            gizmoVerts[startVert + 2] = { (axisVector * 120) + (Vector3D::UP * 10) - (Vector3D::RIGHT * 8), color };
            gizmoVerts[startVert + 3] = { (axisVector * 120) - (Vector3D::UP * 10) + (Vector3D::RIGHT * 8), color };
            gizmoIndices[startIdx + 2] = startVert + 2;
            gizmoIndices[startIdx + 3] = startVert + 3;
            break;
        }
        case 1:
        {
            axisVector = Vector3D::RIGHT;
            otheAxis1 = Vector3D::UP;
            otheAxis2 = Vector3D::FWD;
            color = ColorConst::GREEN;

            // Letter Y
            const uint32 startVert = 22;
            const uint32 startIdx = 34;
            gizmoVerts[startVert] = { (axisVector * 120), color };

            gizmoVerts[startVert + 1] = { (axisVector * 120) + (Vector3D::UP * 10) + (Vector3D::FWD * 8), color };
            gizmoVerts[startVert + 2] = { (axisVector * 120) + (Vector3D::UP * 10) - (Vector3D::FWD * 8), color };
            gizmoVerts[startVert + 3] = { (axisVector * 120) - (Vector3D::UP * 8), color };

            gizmoIndices[startIdx + 0] = startVert;
            gizmoIndices[startIdx + 1] = startVert + 1;
            gizmoIndices[startIdx + 2] = startVert;
            gizmoIndices[startIdx + 3] = startVert + 2;
            gizmoIndices[startIdx + 4] = startVert;
            gizmoIndices[startIdx + 5] = startVert + 3;
            break;
        }
        case 2:
        {
            axisVector = Vector3D::UP;
            otheAxis1 = Vector3D::FWD;
            otheAxis2 = Vector3D::RIGHT;
            color = ColorConst::BLUE;

            // Letter Z
            const uint32 startVert = 26;
            const uint32 startIdx = 40;

            gizmoVerts[startVert + 0] = { (axisVector * 130) + (Vector3D::UP * 9) + (Vector3D::RIGHT * 7), color };
            gizmoVerts[startVert + 1] = { (axisVector * 130) + (Vector3D::UP * 9) - (Vector3D::RIGHT * 7), color };
            gizmoVerts[startVert + 2] = { (axisVector * 130) - (Vector3D::UP * 9) + (Vector3D::RIGHT * 7), color };
            gizmoVerts[startVert + 3] = { (axisVector * 130) - (Vector3D::UP * 9) - (Vector3D::RIGHT * 7), color };

            gizmoIndices[startIdx + 0] = startVert;
            gizmoIndices[startIdx + 1] = startVert + 1;
            gizmoIndices[startIdx + 2] = startVert + 1;
            gizmoIndices[startIdx + 3] = startVert + 2;
            gizmoIndices[startIdx + 4] = startVert + 2;
            gizmoIndices[startIdx + 5] = startVert + 3;
            break;
        }
        }

        // Axis line
        uint32 idx = axis * idxPerAxis + 0;
        gizmoVerts[idx] = { Vector3D::ZERO, color };
        gizmoIndices[axis * vertPerAxis + 0] = idx;

        idx = axis * idxPerAxis + 1;
        gizmoVerts[idx] = { axisVector * 100, color };
        gizmoIndices[axis * vertPerAxis + 1] = idx;

        // Arrow along plane
        // All mid points of arrow
        gizmoIndices[axis * vertPerAxis + 2] = gizmoIndices[axis * vertPerAxis + 4] = gizmoIndices[axis * vertPerAxis + 6] = gizmoIndices[axis * vertPerAxis + 8] = idx;

        const Vector3D& startPos = gizmoVerts[idx].position;
        idx = axis * idxPerAxis + 2;
        gizmoVerts[idx] = { startPos + (otheAxis1 + otheAxis2 - axisVector).normalized() * 10, color };
        gizmoIndices[axis * vertPerAxis + 3] = idx;
        idx = axis * idxPerAxis + 3;
        gizmoVerts[idx] = { startPos + (otheAxis1 - otheAxis2 - axisVector).normalized() * 10, color };
        gizmoIndices[axis * vertPerAxis + 5] = idx;
        idx = axis * idxPerAxis + 4;
        gizmoVerts[idx] = { startPos - (otheAxis1 - otheAxis2 + axisVector).normalized() * 10, color };
        gizmoIndices[axis * vertPerAxis + 7] = idx;
        idx = axis * idxPerAxis + 5;
        gizmoVerts[idx] = { startPos - (otheAxis1 + otheAxis2 + axisVector).normalized() * 10, color };
        gizmoIndices[axis * vertPerAxis + 9] = idx;
    }

    BufferResourceRef lineGizmoVertsBuffer = graphicsHelper->createReadOnlyVertexBuffer(graphicsInstance, sizeof(VertexSimple3DColor), static_cast<uint32>(gizmoVerts.size()));
    lineGizmoVertsBuffer->setResourceName("LineGizmosVertices");
    lineGizmoVertsBuffer->init();

    BufferResourceRef lineGizmoIndicesBuffer = graphicsHelper->createReadOnlyIndexBuffer(graphicsInstance, sizeof(uint32), static_cast<uint32>(gizmoIndices.size()));
    lineGizmoIndicesBuffer->setResourceName("LineGizmosIndices");
    lineGizmoIndicesBuffer->init();

    GlobalBuffers::lineGizmoVertxInds.first = lineGizmoVertsBuffer;
    GlobalBuffers::lineGizmoVertxInds.second = lineGizmoIndicesBuffer;

    BufferResourceRef quadTriVertexBuffer = graphicsHelper->createReadOnlyVertexBuffer(graphicsInstance, sizeof(Vector3D), static_cast<uint32>(quadTriVerts.size()));
    quadTriVertexBuffer->setResourceName("ScreenQuadTriVertices");
    quadTriVertexBuffer->init();

    GlobalBuffers::quadTriVerts = quadTriVertexBuffer;

    BufferResourceRef quadRectVertexBuffer = graphicsHelper->createReadOnlyVertexBuffer(graphicsInstance, sizeof(Vector3D), static_cast<uint32>(quadRectVerts.size()));
    quadRectVertexBuffer->setResourceName("ScreenQuadRectVertices");
    quadRectVertexBuffer->init();

    BufferResourceRef quadRectIndexBuffer = graphicsHelper->createReadOnlyIndexBuffer(graphicsInstance, sizeof(uint32), static_cast<uint32>(quadRectIndices.size()));
    quadRectIndexBuffer->setResourceName("ScreenQuadRectIndices");
    quadRectIndexBuffer->init();

    GlobalBuffers::quadRectVertsInds.first = quadRectVertexBuffer;
    GlobalBuffers::quadRectVertsInds.second = quadRectIndexBuffer;

    std::vector<BatchCopyBufferData> copies{
        { quadTriVertexBuffer, 0, quadTriVerts.data(), uint32(quadTriVertexBuffer->getResourceSize()) },
        { quadRectVertexBuffer, 0, quadRectVerts.data(), uint32(quadRectVertexBuffer->getResourceSize()) },
        { quadRectIndexBuffer, 0, quadRectIndices.data(), uint32(quadRectIndexBuffer->getResourceSize()) },
        { lineGizmoVertsBuffer, 0, gizmoVerts.data(), uint32(lineGizmoVertsBuffer->getResourceSize()) },
        { lineGizmoIndicesBuffer, 0, gizmoIndices.data(), uint32(lineGizmoIndicesBuffer->getResourceSize()) }
    };
    cmdList->copyToBuffer(copies);
}
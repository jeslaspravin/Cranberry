#include "StaticMeshAsset.h"
#include "../../RenderInterface/PlatformIndependentHeaders.h"
#include "../../RenderInterface/PlatformIndependentHelper.h"
#include "../../Core/Engine/GameEngine.h"
#include "../../RenderInterface/Rendering/IRenderCommandList.h"

BEGIN_VERTEX_DEFINITION(StaticMeshVertex)
ADD_VERTEX_FIELD(position)
ADD_VERTEX_FIELD(normal)
ADD_VERTEX_FIELD(vertexColor)
END_VERTEX_DEFINITION();

void StaticMeshAsset::initAsset()
{
    ENQUEUE_COMMAND(InitializeSMVertices,
        {
            vertexBuffer = new GraphicsVertexBuffer(getShaderParamInfo<StaticMeshAsset>()->paramStride(), uint32(vertices.size()));
            vertexBuffer->init();
            cmdList->copyToBuffer(vertexBuffer, 0, vertices.data(), uint32(vertexBuffer->getResourceSize()));

            indexBuffer = new GraphicsIndexBuffer(sizeof(uint32), uint32(indices.size()));
            indexBuffer->init();
            cmdList->copyToBuffer(indexBuffer, 0, indices.data(), uint32(indexBuffer->getResourceSize()));
        }
        , this);
}

void StaticMeshAsset::clearAsset()
{
    ENQUEUE_COMMAND(ClearSMVertices,
        { 
            vertexBuffer->release();
            delete vertexBuffer;
            indexBuffer->release();
            delete indexBuffer;
        }
    , this);
}

template<>
ShaderVertexParamInfo* MeshAsset::getShaderParamInfo<StaticMeshAsset>()
{
    static StaticMeshVertexVertexParamInfo vertexParamInfo;
    return &vertexParamInfo;
}
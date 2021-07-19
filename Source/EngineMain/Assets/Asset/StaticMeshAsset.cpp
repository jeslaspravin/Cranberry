#include "StaticMeshAsset.h"
#include "../../RenderInterface/PlatformIndependentHeaders.h"
#include "../../RenderInterface/PlatformIndependentHelper.h"
#include "../../Core/Engine/GameEngine.h"
#include "../../RenderInterface/Rendering/IRenderCommandList.h"
#include "../../RenderApi/VertexData.h"

void StaticMeshAsset::initAsset()
{
    ENQUEUE_COMMAND(InitializeSMVertices, LAMBDA_BODY
        (
            vertexBuffer = new GraphicsVertexBuffer(EVertexType::vertexParamInfo<EVertexType::StaticMesh>()[0]->paramStride()
                , uint32(vertices.size()));
            vertexBuffer->setResourceName(assetHeader.assetName + "_VertexBuffer");
            vertexBuffer->init();
            cmdList->copyToBuffer(vertexBuffer, 0, vertices.data(), uint32(vertexBuffer->getResourceSize()));

            indexBuffer = new GraphicsIndexBuffer(sizeof(uint32), uint32(indices.size()));
            indexBuffer->setResourceName(assetHeader.assetName + "_IndexBuffer");
            indexBuffer->init();
            cmdList->copyToBuffer(indexBuffer, 0, indices.data(), uint32(indexBuffer->getResourceSize()));
        )
        , this);

#if _DEBUG
    ENQUEUE_COMMAND(InitializeSMTbnVertices, LAMBDA_BODY
    (
        tbnVertexBuffer = new GraphicsVertexBuffer(sizeof(TbnLinePoint), uint32(tbnVerts.size()));
        tbnVertexBuffer->setResourceName(assetHeader.assetName + "_TbnVertexBuffer");
        tbnVertexBuffer->init();
        cmdList->copyToBuffer(tbnVertexBuffer, 0, tbnVerts.data(), uint32(tbnVertexBuffer->getResourceSize()));
    )
        , this);
#endif
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

#if _DEBUG
    ENQUEUE_COMMAND(InitializeSMTbnVertices, LAMBDA_BODY
    (
        tbnVertexBuffer->release();
        delete tbnVertexBuffer;
    ), this);
#endif
}
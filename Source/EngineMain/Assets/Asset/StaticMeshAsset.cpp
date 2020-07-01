#include "StaticMeshAsset.h"
#include "../../RenderInterface/PlatformIndependentHeaders.h"
#include "../../RenderInterface/PlatformIndependentHelper.h"
#include "../../Core/Engine/GameEngine.h"
#include "../../RenderInterface/Rendering/IRenderCommandList.h"
#include "../../RenderApi/VertexData.h"

void StaticMeshAsset::initAsset()
{
    ENQUEUE_COMMAND(InitializeSMVertices,
        {
            vertexBuffer = new GraphicsVertexBuffer(EVertexType::vertexParamInfo<EVertexType::StaticMesh>()[0]->paramStride()
                , uint32(vertices.size()));
            vertexBuffer->setResourceName(assetHeader.assetName + "_VertexBuffer");
            vertexBuffer->init();
            cmdList->copyToBuffer(vertexBuffer, 0, vertices.data(), uint32(vertexBuffer->getResourceSize()));

            indexBuffer = new GraphicsIndexBuffer(sizeof(uint32), uint32(indices.size()));
            indexBuffer->setResourceName(assetHeader.assetName + "_IndexBuffer");
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
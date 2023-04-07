/*!
 * \file StaticMeshAsset.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "Assets/Asset/StaticMeshAsset.h"
#include "Engine/TestGameEngine.h"
#include "RenderApi/VertexData.h"
#include "RenderApi/RenderTaskHelpers.h"
#include "RenderInterface/GraphicsHelper.h"
#include "RenderInterface/Rendering/IRenderCommandList.h"

void StaticMeshAsset::initAsset()
{
    ENQUEUE_RENDER_COMMAND(InitializeSMVertices)
    (
        [this](IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper)
        {
            vertexBuffer = graphicsHelper->createReadOnlyVertexBuffer(
                graphicsInstance, EVertexType::vertexParamInfo<EVertexType::StaticMesh>()[0]->paramStride(), uint32(vertices.size())
            );
            vertexBuffer->setResourceName(assetHeader.assetName + TCHAR("_VertexBuffer"));
            vertexBuffer->init();
            cmdList->copyToBuffer(vertexBuffer, 0, vertices.data(), uint32(vertexBuffer->getResourceSize()));

            indexBuffer = graphicsHelper->createReadOnlyIndexBuffer(graphicsInstance, sizeof(uint32), uint32(indices.size()));
            indexBuffer->setResourceName(assetHeader.assetName + TCHAR("_IndexBuffer"));
            indexBuffer->init();
            cmdList->copyToBuffer(indexBuffer, 0, indices.data(), uint32(indexBuffer->getResourceSize()));
        }
    );

#if DEV_BUILD
    ENQUEUE_RENDER_COMMAND(InitializeSMTbnVertices)
    (
        [this](IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance, const GraphicsHelperAPI *graphicsHelper)
        {
            tbnVertexBuffer = graphicsHelper->createReadOnlyVertexBuffer(graphicsInstance, sizeof(TbnLinePoint), uint32(tbnVerts.size()));
            tbnVertexBuffer->setResourceName(assetHeader.assetName + TCHAR("_TbnVertexBuffer"));
            tbnVertexBuffer->init();
            cmdList->copyToBuffer(tbnVertexBuffer, 0, tbnVerts.data(), uint32(tbnVertexBuffer->getResourceSize()));
        }
    );
#endif
}

void StaticMeshAsset::clearAsset()
{
    ENQUEUE_RENDER_COMMAND(ClearSMVertices)
    (
        [this](IRenderCommandList *, IGraphicsInstance *, const GraphicsHelperAPI *)
        {
            vertexBuffer->release();
            vertexBuffer.reset();
            indexBuffer->release();
            indexBuffer.reset();
        }
    );

#if DEV_BUILD
    ENQUEUE_RENDER_COMMAND(InitializeSMTbnVertices)
    (
        [this](IRenderCommandList *, IGraphicsInstance *, const GraphicsHelperAPI *)
        {
            tbnVertexBuffer->release();
            tbnVertexBuffer.reset();
        }
    );
#endif
}
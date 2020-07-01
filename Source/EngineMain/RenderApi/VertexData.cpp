#include "VertexData.h"
#include "../Assets/Asset/StaticMeshAsset.h"
#include "../Core/Platform/PlatformAssertionErrors.h"

BEGIN_VERTEX_DEFINITION(StaticMeshVertex)
ADD_VERTEX_FIELD(position)
ADD_VERTEX_FIELD(normal)
ADD_VERTEX_FIELD(vertexColor)
END_VERTEX_DEFINITION();

namespace EVertexType
{
    template<>
    const std::vector<ShaderVertexParamInfo*>& vertexParamInfo<Simple2>()
    {
        static std::vector<ShaderVertexParamInfo*> VERTEX_PARAMS;
        debugAssert(!"Not implemented");
        return VERTEX_PARAMS;
    }

    template<>
    const std::vector<ShaderVertexParamInfo*>& vertexParamInfo<Simple3>()
    {
        static std::vector<ShaderVertexParamInfo*> VERTEX_PARAMS;
        debugAssert(!"Not implemented");
        return VERTEX_PARAMS;
    }
    template<>
    const std::vector<ShaderVertexParamInfo*>& vertexParamInfo<Simple4>()
    {
        static std::vector<ShaderVertexParamInfo*> VERTEX_PARAMS;
        debugAssert(!"Not implemented");
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

    String toString(Type vertexType)
    {
        switch (vertexType)
        {
        case EVertexType::Simple2:
            return "Simple2d";
        case EVertexType::Simple3:
            return "Simple3d";
            break;
        case EVertexType::Simple4:
            return "Simple";
            break;
        case EVertexType::BasicMesh:
            return "BasicMesh";
            break;
        case EVertexType::StaticMesh:
            return "StaticMesh";
            break;
        }
        return "";
    }

    constexpr const std::vector<ShaderVertexParamInfo*>& vertexParamInfo(Type vertexType)
    {
        switch (vertexType)
        {
        case EVertexType::Simple2:
            return vertexParamInfo<Simple2>();
        case EVertexType::Simple3:
            return vertexParamInfo<Simple3>();
        case EVertexType::Simple4:
            return vertexParamInfo<Simple4>();
        default:
        case EVertexType::BasicMesh:
            return vertexParamInfo<BasicMesh>();
        case EVertexType::StaticMesh:
            return vertexParamInfo<StaticMesh>();
        }
    }

}
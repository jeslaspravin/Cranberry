#include "VertexData.h"
#include "../Assets/Asset/StaticMeshAsset.h"
#include "../Core/Platform/PlatformAssertionErrors.h"
#include "../Core/Math/Vector2D.h"
#include "../Core/Math/Vector3D.h"
#include "../Core/Math/Vector4D.h"

BEGIN_VERTEX_DEFINITION(StaticMeshVertex, EShaderInputFrequency::PerVertex)
ADD_VERTEX_FIELD(position)
ADD_VERTEX_FIELD(normal)
ADD_VERTEX_FIELD(vertexColor)
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

struct VertexSimple4D
{
    Vector4D position;
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

BEGIN_VERTEX_DEFINITION(VertexSimple4D, EShaderInputFrequency::PerVertex)
ADD_VERTEX_FIELD(position)
END_VERTEX_DEFINITION();

namespace EVertexType
{
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
        static VertexSimple3DVertexParamInfo STATIC_VERTEX_PARAM_INFO;
        static std::vector<ShaderVertexParamInfo*> VERTEX_PARAMS{ &STATIC_VERTEX_PARAM_INFO };
        return VERTEX_PARAMS;
    }
    template<>
    const std::vector<ShaderVertexParamInfo*>& vertexParamInfo<Simple4>()
    {
        static VertexSimple4DVertexParamInfo STATIC_VERTEX_PARAM_INFO;
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
}
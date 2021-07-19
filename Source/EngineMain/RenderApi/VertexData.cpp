#include "VertexData.h"
#include "../Assets/Asset/StaticMeshAsset.h"
#include "../Core/Platform/PlatformAssertionErrors.h"
#include "../Core/Math/Vector2D.h"
#include "../Core/Math/Vector3D.h"
#include "../Core/Math/Vector4D.h"
#include "ShaderDataTypes.h"

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
        default:
            return vertexParamInfo<BasicMesh>();
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
        default:
        case EVertexType::BasicMesh:
            return vertexSpecConsts<BasicMesh>(specializationConst);
        case EVertexType::StaticMesh:
            return vertexSpecConsts<StaticMesh>(specializationConst);
        case EVertexType::InstancedSimple3DColor:
            return vertexSpecConsts<InstancedSimple3DColor>(specializationConst);
        }
    }

}
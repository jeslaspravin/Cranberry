#include "UtilityShaders.h"
#include "../../../Core/Logger/Logger.h"

EVertexType::Type UniqueUtilityShader::vertexUsage() const
{
    // Since at the moment only planning to have either simple or basic vertices only
    if (getReflection()->inputs.size() == 1)
    {
        switch (getReflection()->inputs[0].data.type.vecSize)
        {
        case 2:
            return EVertexType::Simple2;
        case 3:
            return EVertexType::Simple3;
        case 4:
            return EVertexType::Simple4;
        }
    }
    else if(getReflection()->inputs.size() == 2)
    {
        return EVertexType::BasicMesh;
    }

    Logger::error("UniqueUtilityShader", "%s : not supported vertex format for Utility shader");
    return EVertexType::Simple2;
}

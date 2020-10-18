#include "ShaderParameters.h"

ShaderBufferField::ShaderBufferField(const String& pName)
    : paramName(pName)
{}

ShaderVertexField::ShaderVertexField(const String& attribName, const uint32& offsetVal)
    : attributeName(attribName)
    , offset(offsetVal)
{}
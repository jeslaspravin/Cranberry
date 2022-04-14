/*!
 * \file ShaderParameters.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "RenderInterface/ShaderCore/ShaderParameters.h"

ShaderBufferField::ShaderBufferField(const String &pName)
    : paramName(pName)
{}

ShaderVertexField::ShaderVertexField(const String &attribName, const uint32 &offsetVal)
    : attributeName(attribName)
    , offset(offsetVal)
{}

ShaderVertexField::ShaderVertexField(
    const String &attribName, const uint32 &offsetVal, EShaderInputAttribFormat::Type overrideFormat)
    : attributeName(attribName)
    , offset(offsetVal)
    , format(overrideFormat)
{}

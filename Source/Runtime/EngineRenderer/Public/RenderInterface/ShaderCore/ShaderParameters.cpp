/*!
 * \file ShaderParameters.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "RenderInterface/ShaderCore/ShaderParameters.h"
#include "Types/Platform/PlatformAssertionErrors.h"

ShaderBufferField::ShaderBufferField(const TChar *pName, FieldDecorationFlags decorations)
    : paramName(pName)
    , fieldDecorations(decorations)
{
    alertOncef(NO_BITS_SET(decorations, INFERRED_DECO_FLAGS), "Inferred decoration flags cannot be explicitly set");
    CLEAR_BITS(fieldDecorations, INFERRED_DECO_FLAGS);
}

ShaderVertexField::ShaderVertexField(const TChar *attribName, uint32 offsetVal)
    : attributeName(attribName)
    , offset(offsetVal)
{}

ShaderVertexField::ShaderVertexField(const TChar *attribName, uint32 offsetVal, EShaderInputAttribFormat::Type overrideFormat)
    : attributeName(attribName)
    , offset(offsetVal)
    , format(overrideFormat)
{}

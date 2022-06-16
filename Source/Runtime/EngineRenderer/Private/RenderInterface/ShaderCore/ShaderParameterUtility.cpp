/*!
 * \file ShaderParameterUtility.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "RenderInterface/ShaderCore/ShaderParameterUtility.h"
#include "ShaderReflected.h"
#include "Types/Platform/PlatformAssertionErrors.h"

bool shaderBufferHasAnySpecializationConst(const ReflectBufferShaderField &bufferField)
{
    bool bHasAnySpec = false;

    for (const ReflectBufferEntry &bufferInnerField : bufferField.bufferFields)
    {
        for (const ArrayDefinition &arrayDef : bufferInnerField.data.arraySize)
        {
            bHasAnySpec = bHasAnySpec || arrayDef.isSpecializationConst;
        }
    }

    if (!bHasAnySpec)
    {
        for (const ReflectBufferStructEntry &bufferStructField : bufferField.bufferStructFields)
        {
            for (const ArrayDefinition &arrayDef : bufferStructField.data.arraySize)
            {
                bHasAnySpec = bHasAnySpec || arrayDef.isSpecializationConst;
            }
            bHasAnySpec = bHasAnySpec || shaderBufferHasAnySpecializationConst(bufferStructField.data.data);
        }
    }
    return bHasAnySpec;
}

// Alignments based on
// https://khronos.org/registry/vulkan/specs/1.2-extensions/html/chap16.html#interfaces-resources-layout
uint32 getAlignment(const ReflectFieldType &fieldType)
{
    uint32 alignment = 0;
    switch (fieldType.primitive)
    {
    case ReflectPrimitive_bool:
        alignment = sizeof(bool);
        break;
    case ReflectPrimitive_int:
        alignment = sizeof(int32);
        break;
    case ReflectPrimitive_uint:
        alignment = sizeof(uint32);
        break;
    case ReflectPrimitive_float:
        alignment = sizeof(float);
        break;
    case ReflectPrimitive_double:
        alignment = sizeof(double);
        break;
    case RelectPrimitive_invalid:
    default:
        break;
    }

    // Aligned by 2 for vec3,vec4 both take 4x scalar
    alignment *= fieldType.vecSize > 2 ? Math::alignBy2(fieldType.vecSize) : fieldType.vecSize;
    return alignment;
}

uint32 getAlignment(const ReflectBufferShaderField &bufferField)
{
    uint32 alignment = 0;
    for (const ReflectBufferStructEntry &bufferStructField : bufferField.bufferStructFields)
    {
        uint32 fieldAlignment = getAlignment(bufferStructField.data.data);
        if (alignment < fieldAlignment)
        {
            alignment = fieldAlignment;
        }
    }

    for (const ReflectBufferEntry &bufferInnerField : bufferField.bufferFields)
    {
        uint32 fieldAlignment = getAlignment(bufferInnerField.data.data.type);
        if (alignment < fieldAlignment)
        {
            alignment = fieldAlignment;
        }
    }
    // Align 16
    alignment = Math::alignBy(alignment, 16u);
    return alignment;
}

bool ShaderParameterUtility::fillRefToBufParamInfo(
    ShaderBufferParamInfo &bufferParamInfo, const ReflectBufferShaderField &bufferField,
    const std::vector<std::vector<SpecializationConstantEntry>> &stageSpecializationConsts
)
{
    bool bHasRuntimeArray = false;
    uint32 runtimeArrayOffset = 0;
    if (shaderBufferHasAnySpecializationConst(bufferField))
    {
        std::map<String, ShaderBufferField *> attribNameToNode;
        {
            for (ShaderBufferField *currentField : bufferParamInfo)
            {
                attribNameToNode[currentField->paramName] = currentField;
            }
        }

        uint32 bufferSize = 0;
        int32 structIdx = 0, innerFieldIdx = 0;
        while (structIdx < bufferField.bufferStructFields.size() || innerFieldIdx < bufferField.bufferFields.size())
        {
            // If buffer field is before inner field
            if (structIdx < bufferField.bufferStructFields.size()
                && (innerFieldIdx >= bufferField.bufferFields.size()
                    || bufferField.bufferFields[innerFieldIdx].data.offset > bufferField.bufferStructFields[structIdx].data.offset))
            {
                const ReflectBufferStructEntry &bufferStructField = bufferField.bufferStructFields[structIdx];

                auto currFieldItr = attribNameToNode.find(UTF8_TO_TCHAR(bufferStructField.attributeName.c_str()));
                debugAssert(
                    currFieldItr != attribNameToNode.end() && BIT_SET(currFieldItr->second->fieldDecorations, ShaderBufferField::IsStruct)
                );
                ShaderBufferField *currentField = currFieldItr->second;

                // If there is runtime array already at earlier offset then fail as we allow
                // runtime to be always last entry in buffer
                debugAssert(!bHasRuntimeArray);
                // We don't have to do offset check here as we already go member by member
                // sequentially
                bHasRuntimeArray = (bufferStructField.data.totalSize == 0 && currentField->isPointer());
                // Make sure we do not miss intended runtime array
                debugAssert(bufferStructField.data.totalSize != 0 && !currentField->isPointer());

                // Not array dimension greater than 1 or if runtime array
                debugAssert(bufferStructField.data.arraySize.size() == 1);
                debugAssert(
                    !currentField->isIndexAccessible() || bHasRuntimeArray || (bufferStructField.data.arraySize[0].dimension > 1)
                    || bufferStructField.data.arraySize[0].isSpecializationConst
                );

                // default C++ length
                const uint32 arrayLength
                    = currentField->isIndexAccessible() && !currentField->isPointer() ? (currentField->size / currentField->stride) : 1;
                const uint32 alignment = getAlignment(bufferStructField.data.data);
                bufferSize = Math::alignBy(bufferSize, alignment);

                fillRefToBufParamInfo(*currentField->paramInfo, bufferStructField.data.data, stageSpecializationConsts);
                currentField->offset = bufferSize;
                currentField->stride = currentField->paramInfo->paramStride();
                if (bufferStructField.data.arraySize[0].isSpecializationConst)
                {
                    uint32 specializedLength = 0;
                    SpecializationConstUtility::asValue(
                        specializedLength,
                        stageSpecializationConsts[bufferStructField.data.arraySize[0].stageIdx][bufferStructField.data.arraySize[0].dimension]
                    );
                    debugAssert(arrayLength == specializedLength);
                    currentField->size = currentField->stride * specializedLength;
                }
                else
                {
                    currentField->size = currentField->stride * arrayLength;
                }

                bufferSize += currentField->size;
                ++structIdx;
            }
            else
            {
                const ReflectBufferEntry &bufferInnerField = bufferField.bufferFields[innerFieldIdx];

                auto currFieldItr = attribNameToNode.find(UTF8_TO_TCHAR(bufferInnerField.attributeName.c_str()));
                debugAssert(
                    currFieldItr != attribNameToNode.end() && BIT_SET(currFieldItr->second->fieldDecorations, ShaderBufferField::IsStruct)
                );
                ShaderBufferField *currentField = currFieldItr->second;

                // If there is runtime array already at earlier offset then fail as we allow
                // runtime to be always last entry in buffer
                debugAssert(!bHasRuntimeArray);
                // We don't have to do offset check here as we already go member by member
                // sequentially
                bHasRuntimeArray = (bufferInnerField.data.totalSize == 0 && currentField->isPointer());
                // Make sure we do not miss intended runtime array
                debugAssert(bufferInnerField.data.totalSize != 0 && !currentField->isPointer());

                // Not array dimension greater than 1
                debugAssert(bufferInnerField.data.arraySize.size() == 1);
                debugAssert(
                    !currentField->isIndexAccessible() || bHasRuntimeArray || (bufferInnerField.data.arraySize[0].dimension > 1)
                    || bufferInnerField.data.arraySize[0].isSpecializationConst
                );

                // default C++ length
                uint32 arrayLength
                    = currentField->isIndexAccessible() && !currentField->isPointer() ? (currentField->size / currentField->stride) : 0;
                const uint32 alignment = getAlignment(bufferInnerField.data.data.type);
                bufferSize = Math::alignBy(bufferSize, alignment);

                currentField->offset = bufferSize;
                currentField->stride = bufferInnerField.data.stride;
                // Since in case of runtime array total size will be 0
                currentField->size = currentField->isPointer() ? currentField->size : bufferInnerField.data.totalSize;
                currentField->fieldType = EShaderInputAttribFormat::getInputFormat(bufferInnerField.data.data.type);
                if (bufferInnerField.data.arraySize[0].isSpecializationConst)
                {
                    uint32 specializedLength = 0;
                    SpecializationConstUtility::asValue(
                        specializedLength,
                        stageSpecializationConsts[bufferInnerField.data.arraySize[0].stageIdx][bufferInnerField.data.arraySize[0].dimension]
                    );
                    debugAssert(arrayLength == specializedLength);
                    currentField->size = currentField->stride * specializedLength;
                }

                bufferSize += currentField->size;
                ++innerFieldIdx;
            }
        }
        bufferParamInfo.setStride(bufferSize);
    }
    else
    {
        bufferParamInfo.setStride(bufferField.stride);
        for (ShaderBufferField *currentField : bufferParamInfo)
        {
            if (BIT_SET(currentField->fieldDecorations, ShaderBufferField::IsStruct))
            {
                for (const ReflectBufferStructEntry &bufferStructField : bufferField.bufferStructFields)
                {
                    String reflectStructFieldName{ UTF8_TO_TCHAR(bufferStructField.attributeName.c_str()) };
                    if (reflectStructFieldName == currentField->paramName)
                    {
                        // If runtime array then update offset, else check offset to
                        // be sure that runtime array is last member
                        if ((bufferStructField.data.totalSize == 0 && currentField->isPointer()))
                        {
                            bHasRuntimeArray = true;
                            runtimeArrayOffset = bufferStructField.data.offset;
                        }
                        else
                        {
                            // Make sure we do not miss intended runtime array
                            debugAssert(bufferStructField.data.totalSize != 0 && !currentField->isPointer());
                            fatalAssertf(
                                !bHasRuntimeArray || (runtimeArrayOffset >= bufferStructField.data.offset),
                                "Runtime array(offset : %d) must be the "
                                "last member of SoA. Member %s offset %d",
                                runtimeArrayOffset, reflectStructFieldName, bufferStructField.data.offset
                            );
                        }

                        // Not array dimension greater than 1
                        debugAssert(bufferStructField.data.arraySize.size() == 1);
                        debugAssert(
                            !currentField->isIndexAccessible() || bHasRuntimeArray || (bufferStructField.data.arraySize[0].dimension > 1)
                        );

                        currentField->offset = bufferStructField.data.offset;
                        currentField->stride = bufferStructField.data.stride;
                        currentField->size = bufferStructField.data.totalSize;
                        fillRefToBufParamInfo(*currentField->paramInfo, bufferStructField.data.data, stageSpecializationConsts);
                    }
                }
            }
            else
            {
                for (const ReflectBufferEntry &bufferInnerField : bufferField.bufferFields)
                {
                    String reflectFieldName{ UTF8_TO_TCHAR(bufferInnerField.attributeName.c_str()) };
                    if (reflectFieldName == currentField->paramName)
                    {
                        // If runtime array then update offset, else check offset to
                        // be sure that runtime array is last member
                        if ((bufferInnerField.data.totalSize == 0 && currentField->isPointer()))
                        {
                            bHasRuntimeArray = true;
                            runtimeArrayOffset = bufferInnerField.data.offset;
                        }
                        else
                        {
                            // Make sure we do not miss intended runtime array
                            debugAssert(bufferInnerField.data.totalSize != 0 && !currentField->isPointer());
                            fatalAssertf(
                                !bHasRuntimeArray || (runtimeArrayOffset >= bufferInnerField.data.offset),
                                "Runtime array(offset : %d) must be the "
                                "last member of SoA. Member %s offset %d",
                                runtimeArrayOffset, reflectFieldName, bufferInnerField.data.offset
                            );
                        }

                        // Not array dimension greater than 1
                        debugAssert(bufferInnerField.data.arraySize.size() == 1);
                        debugAssert(
                            !currentField->isIndexAccessible() || bHasRuntimeArray || (bufferInnerField.data.arraySize[0].dimension > 1)
                        );

                        currentField->offset = bufferInnerField.data.offset;
                        currentField->stride = bufferInnerField.data.stride;
                        currentField->size = bufferInnerField.data.totalSize;
                        currentField->fieldType = EShaderInputAttribFormat::getInputFormat(bufferInnerField.data.data.type);
                    }
                }
            }
        }
    }

    // If runtime array SoA, Then we need 0 stride just to be in sync with reflection
    if (bHasRuntimeArray)
    {
        bufferParamInfo.setStride(0);
    }
    return true;
}

bool ShaderParameterUtility::fillRefToVertexParamInfo(
    ShaderVertexParamInfo &vertexParamInfo, const std::vector<ReflectInputOutput> &inputEntries
)
{
    for (ShaderVertexField *currentField : vertexParamInfo)
    {
        for (const ReflectInputOutput &vertexAttribute : inputEntries)
        {
            String reflectVertAttribName{ UTF8_TO_TCHAR(vertexAttribute.attributeName.c_str()) };
            if (reflectVertAttribName == currentField->attributeName)
            {
                currentField->location = vertexAttribute.data.location;
                if (currentField->format == EShaderInputAttribFormat::Undefined)
                {
                    currentField->format = EShaderInputAttribFormat::getInputFormat(vertexAttribute.data.type);
                }
            }
        }
    }
    return true;
}

uint32 ShaderParameterUtility::convertNamedSpecConstsToPerStage(
    std::vector<std::vector<SpecializationConstantEntry>> &stageSpecializationConsts,
    const std::map<String, SpecializationConstantEntry> &namedSpecializationConsts, const struct ShaderReflected *shaderReflection
)
{
    stageSpecializationConsts.resize(shaderReflection->stages.size());

    uint32 specConstsCount = 0;
    uint32 stageIdx = 0;
    for (const ShaderStageDescription &stageDesc : shaderReflection->stages)
    {
        for (const ReflectSpecializationConstant &stageSpecConst : stageDesc.stageSpecializationEntries)
        {
            String specConstAttribName{ UTF8_TO_TCHAR(stageSpecConst.attributeName.c_str()) };
            std::map<String, SpecializationConstantEntry>::const_iterator specConstVal = namedSpecializationConsts.find(specConstAttribName);
            if (specConstVal != namedSpecializationConsts.cend())
            {
                SpecializationConstantEntry &entry = stageSpecializationConsts[stageIdx].emplace_back(specConstVal->second);
                entry.constantId = stageSpecConst.data.constantId;
            }
            else
            {
                if (!stageSpecConst.attributeName.empty())
                {
                    LOG_WARN(
                        "ShaderSetParametersLayout",
                        "No specialization const value found for %s, using "
                        "default",
                        specConstAttribName
                    );
                }
                stageSpecializationConsts[stageIdx].emplace_back(stageSpecConst.data);
            }
            ++specConstsCount;
        }
        ++stageIdx;
    }
    return specConstsCount;
}

std::map<String, uint32> &ShaderParameterUtility::unboundArrayResourcesCount()
{
    static std::map<String, uint32> RUNTIME_BOUND_RESOURCES;
    return RUNTIME_BOUND_RESOURCES;
}

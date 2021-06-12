#include "ShaderParameterUtility.h"
#include "../../Core/Platform/PlatformAssertionErrors.h"
#include "ShaderReflected.h"

bool shaderBufferHasAnySpecializationConst(const ReflectBufferShaderField& bufferField)
{
    bool bHasAnySpec = false;

    for (const ReflectBufferEntry& bufferInnerField : bufferField.bufferFields)
    {
        for (const ArrayDefinition& arrayDef : bufferInnerField.data.arraySize)
        {
            bHasAnySpec = bHasAnySpec || arrayDef.isSpecializationConst;
        }
    }

    if (!bHasAnySpec)
    {
        for (const ReflectBufferStructEntry& bufferStructField : bufferField.bufferStructFields)
        {
            for (const ArrayDefinition& arrayDef : bufferStructField.data.arraySize)
            {
                bHasAnySpec = bHasAnySpec || arrayDef.isSpecializationConst;
            }
            bHasAnySpec = bHasAnySpec || shaderBufferHasAnySpecializationConst(bufferStructField.data.data);
        }
    }
    return bHasAnySpec;
}

// Alignments based on https://khronos.org/registry/vulkan/specs/1.2-extensions/html/chap16.html#interfaces-resources-layout
uint32 getAlignment(const ReflectFieldType& fieldType)
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
    alignment *= fieldType.vecSize > 2 ? (fieldType.vecSize + 1u) & ~1u : fieldType.vecSize;
    return alignment;
}

uint32 getAlignment(const ReflectBufferShaderField& bufferField)
{
    uint32 alignment = 0;
    for (const ReflectBufferStructEntry& bufferStructField : bufferField.bufferStructFields)
    {
        uint32 fieldAlignment = getAlignment(bufferStructField.data.data);
        if (alignment < fieldAlignment)
        {
            alignment = fieldAlignment;
        }
    }

    for (const ReflectBufferEntry& bufferInnerField : bufferField.bufferFields)
    {
        uint32 fieldAlignment = getAlignment(bufferInnerField.data.data.type);
        if (alignment < fieldAlignment)
        {
            alignment = fieldAlignment;
        }
    }
    // Align 16
    alignment = (alignment + 15u) & ~15u;
    return alignment;
}

bool ShaderParameterUtility::fillRefToBufParamInfo(ShaderBufferParamInfo& bufferParamInfo, const ReflectBufferShaderField& bufferField, const std::vector<std::vector<SpecializationConstantEntry>>& stageSpecializationConsts)
{
    if (shaderBufferHasAnySpecializationConst(bufferField))
    {
        std::map<String, ShaderBufferFieldNode*> attribNameToNode;
        {
            ShaderBufferFieldNode* currentFieldNode = &bufferParamInfo.startNode;
            while (currentFieldNode->isValid())
            {
                attribNameToNode[currentFieldNode->field->paramName] = currentFieldNode;
                currentFieldNode = currentFieldNode->nextNode;
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
                const ReflectBufferStructEntry& bufferStructField = bufferField.bufferStructFields[structIdx];

                auto currFieldNodeItr = attribNameToNode.find(bufferStructField.attributeName);
                debugAssert(currFieldNodeItr != attribNameToNode.end() && currFieldNodeItr->second->field->bIsStruct);
                ShaderBufferFieldNode* currentFieldNode = currFieldNodeItr->second;

                // Not array dimension greater than 1
                debugAssert(bufferStructField.data.arraySize.size() == 1);
                debugAssert(!currentFieldNode->field->bIsArray || (bufferStructField.data.arraySize[0].dimension > 1) || bufferStructField.data.arraySize[0].isSpecializationConst);

                // default C++ length
                const uint32 arrayLength = currentFieldNode->field->bIsArray ? (currentFieldNode->field->size / currentFieldNode->field->stride) : 1;
                const uint32 alignment = getAlignment(bufferStructField.data.data);
                bufferSize = (bufferSize + (alignment - 1)) & ~(alignment - 1);

                fillRefToBufParamInfo(*currentFieldNode->field->paramInfo, bufferStructField.data.data, stageSpecializationConsts);
                currentFieldNode->field->offset = bufferSize;
                currentFieldNode->field->stride = currentFieldNode->field->paramInfo->paramStride();
                if (bufferStructField.data.arraySize[0].isSpecializationConst)
                {
                    uint32 specializedLength = 0;
                    SpecializationConstUtility::asValue(specializedLength
                        , stageSpecializationConsts[bufferStructField.data.arraySize[0].stageIdx][bufferStructField.data.arraySize[0].dimension]);
                    debugAssert(arrayLength == specializedLength);
                    currentFieldNode->field->size = currentFieldNode->field->stride * specializedLength;
                }
                else
                {
                    currentFieldNode->field->size = currentFieldNode->field->stride * arrayLength;
                }
                bufferSize += currentFieldNode->field->size;
                ++structIdx;
            }
            else
            {
                const ReflectBufferEntry& bufferInnerField = bufferField.bufferFields[innerFieldIdx];

                auto currFieldNodeItr = attribNameToNode.find(bufferInnerField.attributeName);
                debugAssert(currFieldNodeItr != attribNameToNode.end() && currFieldNodeItr->second->field->bIsStruct);
                ShaderBufferFieldNode* currentFieldNode = currFieldNodeItr->second;

                // Not array dimension greater than 1
                debugAssert(bufferInnerField.data.arraySize.size() == 1);
                debugAssert(!currentFieldNode->field->bIsArray || (bufferInnerField.data.arraySize[0].dimension > 1) || bufferInnerField.data.arraySize[0].isSpecializationConst);

                // default C++ length
                uint32 arrayLength = currentFieldNode->field->bIsArray ? (currentFieldNode->field->size / currentFieldNode->field->stride) : 0;
                const uint32 alignment = getAlignment(bufferInnerField.data.data.type);
                bufferSize = (bufferSize + (alignment - 1)) & ~(alignment - 1);

                currentFieldNode->field->offset = bufferSize;
                currentFieldNode->field->stride = bufferInnerField.data.stride;
                currentFieldNode->field->size = bufferInnerField.data.totalSize;
                currentFieldNode->field->fieldType = EShaderInputAttribFormat::getInputFormat(bufferInnerField.data.data.type);
                if (bufferInnerField.data.arraySize[0].isSpecializationConst)
                {
                    uint32 specializedLength = 0;
                    SpecializationConstUtility::asValue(specializedLength
                        , stageSpecializationConsts[bufferInnerField.data.arraySize[0].stageIdx][bufferInnerField.data.arraySize[0].dimension]);
                    debugAssert(arrayLength == specializedLength);
                    currentFieldNode->field->size = currentFieldNode->field->stride * specializedLength;
                }

                bufferSize += currentFieldNode->field->size;
                ++innerFieldIdx;
            }
        }
        bufferParamInfo.setStride(bufferSize);
    }
    else
    {
        bufferParamInfo.setStride(bufferField.stride);

        ShaderBufferFieldNode* currentFieldNode = &bufferParamInfo.startNode;
        while (currentFieldNode->isValid())
        {
            if (currentFieldNode->field->bIsStruct)
            {
                for (const ReflectBufferStructEntry& bufferStructField : bufferField.bufferStructFields)
                {
                    if (bufferStructField.attributeName == currentFieldNode->field->paramName)
                    {
                        // Not array dimension greater than 1
                        debugAssert(bufferStructField.data.arraySize.size() == 1);
                        debugAssert(!currentFieldNode->field->bIsArray || (bufferStructField.data.arraySize[0].dimension > 1));

                        currentFieldNode->field->offset = bufferStructField.data.offset;
                        currentFieldNode->field->stride = bufferStructField.data.stride;
                        currentFieldNode->field->size = bufferStructField.data.totalSize;
                        fillRefToBufParamInfo(*currentFieldNode->field->paramInfo, bufferStructField.data.data, stageSpecializationConsts);
                    }
                }
            }
            else
            {
                for (const ReflectBufferEntry& bufferInnerField : bufferField.bufferFields)
                {
                    if (bufferInnerField.attributeName == currentFieldNode->field->paramName)
                    {
                        // Not array dimension greater than 1
                        debugAssert(bufferInnerField.data.arraySize.size() == 1);
                        debugAssert(!currentFieldNode->field->bIsArray || (bufferInnerField.data.arraySize[0].dimension > 1));

                        currentFieldNode->field->offset = bufferInnerField.data.offset;
                        currentFieldNode->field->stride = bufferInnerField.data.stride;
                        currentFieldNode->field->size = bufferInnerField.data.totalSize;
                        currentFieldNode->field->fieldType = EShaderInputAttribFormat::getInputFormat(bufferInnerField.data.data.type);
                    }
                }
            }

            currentFieldNode = currentFieldNode->nextNode;
        }
    }
    return true;
}

bool ShaderParameterUtility::fillRefToVertexParamInfo(ShaderVertexParamInfo& vertexParamInfo, const std::vector<ReflectInputOutput>& inputEntries)
{
    ShaderVertexFieldNode* currentFieldNode = &vertexParamInfo.startNode;
    while (currentFieldNode->isValid())
    {
        for (const ReflectInputOutput& vertexAttribute : inputEntries)
        {
            if (vertexAttribute.attributeName == currentFieldNode->field->attributeName)
            {
                currentFieldNode->field->location = vertexAttribute.data.location;
                if (currentFieldNode->field->format == EShaderInputAttribFormat::Undefined)
                {
                    currentFieldNode->field->format = EShaderInputAttribFormat::getInputFormat(vertexAttribute.data.type);
                }
            }
        }
        currentFieldNode = currentFieldNode->nextNode;
    }
    return true;
}

uint32 ShaderParameterUtility::convertNamedSpecConstsToPerStage(std::vector<std::vector<SpecializationConstantEntry>>& stageSpecializationConsts
    , const std::map<String, SpecializationConstantEntry>& namedSpecializationConsts, const struct ShaderReflected* shaderReflection)
{
    stageSpecializationConsts.resize(shaderReflection->stages.size());

    uint32 specConstsCount = 0;
    uint32 stageIdx = 0;
    for (const ShaderStageDescription& stageDesc : shaderReflection->stages)
    {
        for (const ReflectSpecializationConstant& stageSpecConst : stageDesc.stageSpecializationEntries)
        {
            std::map<String, SpecializationConstantEntry>::const_iterator specConstVal = namedSpecializationConsts.find(stageSpecConst.attributeName);
            if (specConstVal != namedSpecializationConsts.cend())
            {
                SpecializationConstantEntry& entry = stageSpecializationConsts[stageIdx].emplace_back(specConstVal->second);
                entry.constantId = stageSpecConst.data.constantId;
            }
            else
            {
                Logger::warn("ShaderSetParametersLayout", "%s() : No specialization const value found for %s, using default", __func__,
                    stageSpecConst.attributeName.c_str());
                stageSpecializationConsts[stageIdx].emplace_back(stageSpecConst.data);
            }
            ++specConstsCount;
        }
        ++stageIdx;
    }
    return specConstsCount;
}

#include "ShaderParameterUtility.h"
#include "../../Core/Platform/PlatformAssertionErrors.h"

bool ShaderParameterUtility::fillRefToBufParamInfo(ShaderBufferParamInfo& bufferParamInfo, const ReflectBufferShaderField& bufferField)
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
                    // Not array dimension greater than 1 and not supporting specialization constant to determine size of array right now
                    debugAssert(bufferStructField.data.arraySize.size() == 1 && !bufferStructField.data.arraySize[0].isSpecializationConst);
                    debugAssert(!currentFieldNode->field->bIsArray || (bufferStructField.data.arraySize[0].dimension > 1));

                    currentFieldNode->field->offset = bufferStructField.data.offset;
                    currentFieldNode->field->stride = bufferStructField.data.stride;
                    currentFieldNode->field->size = bufferStructField.data.totalSize;
                    fillRefToBufParamInfo(*currentFieldNode->field->paramInfo, bufferStructField.data.data);
                }
            }
        }
        else
        {
            for (const ReflectBufferEntry& bufferField : bufferField.bufferFields)
            {
                if (bufferField.attributeName == currentFieldNode->field->paramName)
                {
                    // Not array dimension greater than 1 and not supporting specialization constant to determine size of array right now
                    debugAssert(bufferField.data.arraySize.size() == 1 && !bufferField.data.arraySize[0].isSpecializationConst);
                    debugAssert(!currentFieldNode->field->bIsArray || (bufferField.data.arraySize[0].dimension > 1));

                    currentFieldNode->field->offset = bufferField.data.offset;
                    currentFieldNode->field->stride = bufferField.data.stride;
                    currentFieldNode->field->size = bufferField.data.totalSize;
                    currentFieldNode->field->fieldType = EShaderInputAttribFormat::getInputFormat(bufferField.data.data.type);
                }
            }
        }

        currentFieldNode = currentFieldNode->nextNode;
    }
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
                currentFieldNode->field->format = EShaderInputAttribFormat::getInputFormat(vertexAttribute.data.type);
            }
        }
        currentFieldNode = currentFieldNode->nextNode;
    }
}

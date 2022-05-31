/*!
 * \file ShaderParameterUtility.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once
#include "RenderInterface/ShaderCore/ShaderParameters.h"
#include "ShaderDataTypes.h"

#include <map>

class ENGINERENDERER_EXPORT ShaderParameterUtility
{
public:
    // Some common descriptor set indexes
    constexpr static uint32 BINDLESS_SET = 0;
    constexpr static uint32 VIEW_UNIQ_SET = 1;
    constexpr static uint32 INSTANCE_UNIQ_SET = 2;
    constexpr static uint32 SHADER_UNIQ_SET = 3;
    constexpr static uint32 SHADER_VARIANT_UNIQ_SET = 4;

private:
    ShaderParameterUtility() = default;

public:
    /**
     * ShaderParameterUtility::filRefToBufParamInfo - - Fills the buffer field's offset, size, stride
     * into buffer param info field nodes
     *
     * Access:    public static
     *
     * @param ShaderBufferParamInfo & bufferParamInfo - Buffer info to fill the data into
     * @param const ReflectBufferShaderField & bufferField - reflect buffer field data
     * @param const std::vector<std::vector<SpecializationConstantEntry>>& stageSpecializationConsts -
     * Per stage specialization constants
     *
     * @return bool
     */
    static bool fillRefToBufParamInfo(
        ShaderBufferParamInfo &bufferParamInfo, const ReflectBufferShaderField &bufferField,
        const std::vector<std::vector<SpecializationConstantEntry>> &stageSpecializationConsts
    );

    /**
     * ShaderParameterUtility::filRefToVertexParamInfo - Fills the vertex attributes location and format
     * into vertexParamInfo
     *
     * Access:    public static
     *
     * @param ShaderVertexParamInfo & vertexParamInfo - Vertex info to fill the data into
     * @param const std::vector<ReflectInputOutput> & inputEntries - reflect vertex input attributes to
     * shader
     *
     * @return bool
     */
    static bool fillRefToVertexParamInfo(ShaderVertexParamInfo &vertexParamInfo, const std::vector<ReflectInputOutput> &inputEntries);

    /**
     * ShaderParameterUtility::convertNamedSpecConstsToPerStage - Converts named specialization constants
     * into specialization const per stages
     *
     * Access: public static
     *
     * @param std::vector<std::vector<SpecializationConstantEntry>> & stageSpecializationConsts
     * @param const std::map<String, SpecializationConstantEntry> & namedSpecializationConsts
     * @param const ShaderReflected * shaderReflection
     *
     * @return uint32 - total specialization const across all stages
     */
    static uint32 convertNamedSpecConstsToPerStage(
        std::vector<std::vector<SpecializationConstantEntry>> &stageSpecializationConsts,
        const std::map<String, SpecializationConstantEntry> &namedSpecializationConsts, const struct ShaderReflected *shaderReflection
    );

    static std::map<String, uint32> &unboundArrayResourcesCount();

    /**
     * ShaderParameterUtility::getArrayElementCount - converts the array dimension into linear count,
     * subtitutes unbound array length if needed MaxDimension is to specify whether Array is 1D or nD
     *
     * Access: public static
     *
     * @param const std::vector<ArrayDefinition>& arraySize
     *
     * @return uint32
     */
    template <uint32 MaxDimension>
    static constexpr uint32 getArrayElementCount(
        const String &paramName, const std::vector<ArrayDefinition> &arraySize,
        const std::vector<std::vector<SpecializationConstantEntry>> &specializationConsts
    );
};

template <uint32 MaxDimension>
uint32 constexpr ShaderParameterUtility::getArrayElementCount(
    const String &paramName, const std::vector<ArrayDefinition> &arraySize,
    const std::vector<std::vector<SpecializationConstantEntry>> &specializationConsts
)
{
    if (arraySize.empty())
        return 0;

    uint32 linearCount = 1;
    for (uint32 i = 0; i < MaxDimension && i < arraySize.size(); ++i)
    {
        uint32 count;
        if (!arraySize[i].isSpecializationConst)
        {
            count = arraySize[i].dimension;
        }
        else if (!SpecializationConstUtility::asValue(count, specializationConsts[arraySize[i].stageIdx][arraySize[i].dimension]))
        {
            LOG_ERROR("ShaderParameters", "Specialized %s array count is invalid", paramName.getChar());
        }

        linearCount *= count;
    }

    // if 0 means runtime so check from global constants
    if (linearCount == 0)
    {
        auto itr = std::as_const(ShaderParameterUtility::unboundArrayResourcesCount()).find(paramName);
        linearCount = (itr == ShaderParameterUtility::unboundArrayResourcesCount().cend()) ? 0 : itr->second;
    }
    return linearCount;
}

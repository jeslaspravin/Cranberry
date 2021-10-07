#pragma once
#include "ShaderDataTypes.h"
#include "ShaderParameters.h"

#include <map>

class ShaderParameterUtility
{
private:
    ShaderParameterUtility() = default;
public:
    
    /**
    * ShaderParameterUtility::filRefToBufParamInfo - - Fills the buffer field's offset, size, stride into buffer param info field nodes
    *
    * Access:    public static 
    *
    * @param ShaderBufferParamInfo & bufferParamInfo - Buffer info to fill the data into
    * @param const ReflectBufferShaderField & bufferField - reflect buffer field data
    * @param const std::vector<std::vector<SpecializationConstantEntry>>& stageSpecializationConsts - Per stage specialization constants
    *
    * @return bool
    */
    static bool fillRefToBufParamInfo(ShaderBufferParamInfo& bufferParamInfo, const ReflectBufferShaderField& bufferField
        , const std::vector<std::vector<SpecializationConstantEntry>>& stageSpecializationConsts);

    /**
    * ShaderParameterUtility::filRefToVertexParamInfo - Fills the vertex attributes location and format into vertexParamInfo
    *
    * Access:    public static 
    *
    * @param ShaderVertexParamInfo & vertexParamInfo - Vertex info to fill the data into
    * @param const std::vector<ReflectInputOutput> & inputEntries - reflect vertex input attributes to shader
    *
    * @return bool
    */
    static bool fillRefToVertexParamInfo(ShaderVertexParamInfo& vertexParamInfo, const std::vector<ReflectInputOutput>& inputEntries);

    /**
    * ShaderParameterUtility::convertNamedSpecConstsToPerStage - Converts named specialization constants into specialization const per stages
    *
    * Access: public static  
    *
    * @param std::vector<std::vector<SpecializationConstantEntry>> & stageSpecializationConsts 
    * @param const std::map<String, SpecializationConstantEntry> & namedSpecializationConsts 
    * @param const ShaderReflected * shaderReflection 
    *
    * @return uint32 - total specialization const across all stages
    */
    static uint32 convertNamedSpecConstsToPerStage(std::vector<std::vector<SpecializationConstantEntry>>& stageSpecializationConsts
        , const std::map<String, SpecializationConstantEntry>& namedSpecializationConsts, const struct ShaderReflected* shaderReflection);

    static std::map<String, uint32>& unboundArrayResourcesCount();
};

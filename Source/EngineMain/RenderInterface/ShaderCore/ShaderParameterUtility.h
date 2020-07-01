#pragma once
#include "ShaderDataTypes.h"
#include "ShaderParameters.h"

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
    *
    * @return bool
    */
    static bool fillRefToBufParamInfo(ShaderBufferParamInfo& bufferParamInfo, const ReflectBufferShaderField& bufferField);

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
};

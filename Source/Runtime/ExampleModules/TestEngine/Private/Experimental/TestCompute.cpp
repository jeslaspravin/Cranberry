/*!
 * \file TestCompute.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "Math/Vector2D.h"
#include "Math/Vector4D.h"
#include "RenderInterface/Resources/Pipelines.h"
#include "RenderInterface/ShaderCore/ShaderParameterResources.h"
#include "RenderInterface/Shaders/Base/UtilityShaders.h"
#include "Types/Platform/PlatformAssertionErrors.h"

#define TESTCOMPUTE_SHADER_NAME TCHAR("TestCompute")

struct AOS
{
    Vector4D a;
    Vector2D b;
    Vector2D c[4];
};

struct TestAOS
{
    Vector4D test1;
    AOS *data;
};

BEGIN_BUFFER_DEFINITION(AOS)
ADD_BUFFER_TYPED_FIELD(a)
ADD_BUFFER_TYPED_FIELD(b)
ADD_BUFFER_TYPED_FIELD(c)
END_BUFFER_DEFINITION();

BEGIN_BUFFER_DEFINITION(TestAOS)
ADD_BUFFER_TYPED_FIELD(test1)
ADD_BUFFER_STRUCT_FIELD(data, AOS)
END_BUFFER_DEFINITION();

class TestComputeShader : public ComputeShaderConfig
{
    DECLARE_GRAPHICS_RESOURCE(TestComputeShader, , ComputeShaderConfig, )

public:
    TestComputeShader()
        : BaseType(Byte3D(16, 16, 1), TESTCOMPUTE_SHADER_NAME)
    {}

    void bindBufferParamInfo(std::map<String, struct ShaderBufferDescriptorType *> &bindingBuffers) const override
    {
        static TestAOSBufferParamInfo TESTAOS_INFO;
        static const std::map<String, ShaderBufferParamInfo *> SHADER_PARAMS_INFO{
            {TCHAR("inData"), &TESTAOS_INFO}
        };

        for (const std::pair<const String, ShaderBufferParamInfo *> &bufferInfo : SHADER_PARAMS_INFO)
        {
            auto foundDescBinding = bindingBuffers.find(bufferInfo.first);
            debugAssert(foundDescBinding != bindingBuffers.end());
            foundDescBinding->second->bufferParamInfo = bufferInfo.second;
        }
    }
};

DEFINE_GRAPHICS_RESOURCE(TestComputeShader)

ComputePipelineFactoryRegistrant TESTCOMPUTE_SHADER_PIPELINE_REGISTER(TESTCOMPUTE_SHADER_NAME);

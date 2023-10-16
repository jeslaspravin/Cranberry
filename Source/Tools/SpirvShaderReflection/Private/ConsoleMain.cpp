/*!
 * \file ConsoleMain.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include <iostream>
#include <string>
#include <vector>

#include "EngineShaderSpec/ShaderReflectionProcessor.h"
#include "ShaderArchive.h"
#include "Utilities/CommonFunctions.h"

#if _DEBUG

#include "SpirV/spirv.hpp"
#include "SpirV/spirv_common.hpp"
#include "SpirV/spirv_glsl.hpp"
#include <fstream>

void printArrayCount(const SPIRV_CROSS_NAMESPACE::SPIRType &type)
{
    printf("\t Array count : ");
    if (type.array.empty())
    {
        printf("1\n");
    }
    else
    {
        for (int i = int(type.array.size() - 1); i >= 0; --i)
        {
            printf(
                "[%d : isSpecConstant : %s]\n", type.array[i],
                type.array_size_literal[i] ? "false" : "true"
            ); // SpecConstant is in reverse order of dim array
        }
    }
}

void printMembers(
    const SPIRV_CROSS_NAMESPACE::SPIRType &structType, const SPIRV_CROSS_NAMESPACE::Compiler *compiledData, std::string indent = "\t"
)
{
    uint32_t index = 0;
    for (const SPIRV_CROSS_NAMESPACE::TypeID &memberTypeID : structType.member_types)
    {
        const auto &memberType = compiledData->get_type(memberTypeID);
        if (memberType.basetype == SPIRV_CROSS_NAMESPACE::SPIRType::BaseType::Struct)
        {
            printf(
                "%sStruct : %s Size : %d\n", indent.c_str(), compiledData->get_member_name(structType.self, index).c_str(),
                uint32_t(compiledData->get_declared_struct_member_size(structType, index))
            );
            if (memberType.array.empty())
            {
                printf("%sStride : %d\n", indent.c_str(), uint32_t(compiledData->get_declared_struct_member_size(structType, index)));
            }
            else
            {
                printf("%sStride : %d\n", indent.c_str(), uint32_t(compiledData->type_struct_member_array_stride(structType, index)));
                printArrayCount(memberType);
            }
            printf("%sOffset : %d\n", indent.c_str(), uint32_t(compiledData->type_struct_member_offset(structType, index)));
            printMembers(memberType, compiledData, indent + "\t");
        }
        else
        {
            printf("%sMember : %s\n", indent.c_str(), compiledData->get_member_name(structType.self, index).c_str());
            printf("%sSize : %d\n", indent.c_str(), uint32_t(compiledData->get_declared_struct_member_size(structType, index)));
            if (memberType.columns > 1)
            {
                printf("%sStride : %d\n", indent.c_str(), uint32_t(compiledData->type_struct_member_matrix_stride(structType, index)));
            }
            else if (!memberType.array.empty())
            {
                printf("%sStride : %d\n", indent.c_str(), uint32_t(compiledData->type_struct_member_array_stride(structType, index)));
                printArrayCount(memberType);
            }
            else
            {
                printf("%sStride : %d\n", indent.c_str(), uint32_t(compiledData->get_declared_struct_member_size(structType, index)));
            }
            printf("%sOffset : %d\n", indent.c_str(), uint32_t(compiledData->type_struct_member_offset(structType, index)));
        }
        index++;
    }
}

#endif
/*
 * Arguments must be in order, so that at index 1 to n-3 the file list of all shaders used for this
 * particular processing and graphics pipeline argument at n-2 should be file where the reflected data
 * should be written to argument at n-1 should be file where the combined shader code has to be written
 * to
 */
int main(int argc, char *argv[])
{
    int shaderMaxIndex = argc - 2;

    std::vector<ShaderReflectionProcessor *> reflectionProcessors(shaderMaxIndex - 1);
    for (int i = 1; i < shaderMaxIndex; ++i)
    {
        reflectionProcessors[i - 1] = new ShaderReflectionProcessor(argv[i]);
    }
    PipelineShaderStageProcessor pipelineProcessor(reflectionProcessors, argv[argc - 2], argv[argc - 1]);
    pipelineProcessor.processReflections();
    pipelineProcessor.writeOutput();
#if _DEBUG
    if (!pipelineProcessor.crossCheckWrittenData())
    {
        printf("ERROR: Cross verifying the written files failed\n");
    }
#endif

    for (int i = 0; i < reflectionProcessors.size(); ++i)
    {
        delete reflectionProcessors[i];
    }
    reflectionProcessors.clear();

    // Just some sample codes below
#if 0 & _DEBUG

    // Learning reflection api code down below

    for (int i = 1; i < shaderMaxIndex; ++i)
    {
        std::string fileName = argv[i];
        std::ifstream spvFile = std::ifstream(fileName, std::ios::binary);
        printf("File ----> %s\n", fileName.c_str());
        if (spvFile.is_open())
        {
            spvFile.ignore(std::numeric_limits<std::streamsize>::max());
            std::streamsize length = spvFile.gcount();
            spvFile.clear();   //  Since ignore will have set eof.
            spvFile.seekg(0, std::ifstream::beg);

            std::vector<char> data(length);
            std::vector<uint32_t> shaderData((data.size() + 3) / 4);
            spvFile.read(data.data(), data.size());
            spvFile.close();
            memcpy(shaderData.data(), data.data(), data.size());

            SPIRV_CROSS_NAMESPACE::CompilerGLSL shaderCompiled(std::move(shaderData));
            SPIRV_CROSS_NAMESPACE::ShaderResources shaderResources = shaderCompiled.get_shader_resources();

            for (auto& resource : shaderResources.stage_inputs)
            {
                unsigned location = shaderCompiled.get_decoration(resource.id, spv::DecorationLocation);
                const SPIRV_CROSS_NAMESPACE::SPIRType& basetype = shaderCompiled.get_type(resource.base_type_id);
                //const SPIRV_CROSS_NAMESPACE::SPIRType& type = shaderCompiled.get_type(resource.type_id);

                printf("Stage input %s at location = %u\n", resource.name.c_str(), location);
                printf("\tVector size %d, Column count %d\n", basetype.vecsize, basetype.columns);
            }

            for (auto& resource : shaderResources.stage_outputs)
            {
                unsigned location = shaderCompiled.get_decoration(resource.id, spv::DecorationLocation);
                const SPIRV_CROSS_NAMESPACE::SPIRType& basetype = shaderCompiled.get_type(resource.base_type_id);

                printf("Stage output %s at location = %u\n", resource.name.c_str(), location);
                printf("\tVector size %d, Column count %d\n", basetype.vecsize, basetype.columns);
            }

            for (auto& resource : shaderResources.sampled_images)
            {
                int isTexelBuffer = shaderCompiled.get_type(resource.type_id).image.dim == spv::Dim::DimBuffer;
                unsigned set = shaderCompiled.get_decoration(resource.id, spv::DecorationDescriptorSet);
                unsigned binding = shaderCompiled.get_decoration(resource.id, spv::DecorationBinding);

                printf("Sampled image %s at set = %u binding = %u, isTexel buffer %d\n", resource.name.c_str(), set, binding, isTexelBuffer);

                const SPIRV_CROSS_NAMESPACE::SPIRType& type = shaderCompiled.get_type(resource.type_id);
                printArrayCount(type);
            }
            for (auto& resource : shaderResources.separate_images)
            {
                unsigned set = shaderCompiled.get_decoration(resource.id, spv::DecorationDescriptorSet);
                unsigned binding = shaderCompiled.get_decoration(resource.id, spv::DecorationBinding);

                printf("Texture image %s at set = %u binding = %u\n", resource.name.c_str(), set, binding);

                const SPIRV_CROSS_NAMESPACE::SPIRType& type = shaderCompiled.get_type(resource.type_id);
                printArrayCount(type);
            }

            for (auto& resource : shaderResources.separate_samplers)
            {
                unsigned set = shaderCompiled.get_decoration(resource.id, spv::DecorationDescriptorSet);
                unsigned binding = shaderCompiled.get_decoration(resource.id, spv::DecorationBinding);

                printf("Samplers %s at set = %u binding = %u\n", resource.name.c_str(), set, binding);

                const SPIRV_CROSS_NAMESPACE::SPIRType& type = shaderCompiled.get_type(resource.type_id);
                printArrayCount(type);
            }

            for (auto& resource : shaderResources.storage_images)
            {
                int isTexelBuffer = shaderCompiled.get_type(resource.type_id).image.dim == spv::Dim::DimBuffer;
                unsigned set = shaderCompiled.get_decoration(resource.id, spv::DecorationDescriptorSet);
                unsigned binding = shaderCompiled.get_decoration(resource.id, spv::DecorationBinding);

                printf("Storage image %s at set = %u binding = %u, isTexel buffer %d\n", resource.name.c_str(), set, binding, isTexelBuffer);

                const SPIRV_CROSS_NAMESPACE::SPIRType& type = shaderCompiled.get_type(resource.type_id);
                printArrayCount(type);
            }

            for (auto& resource : shaderResources.uniform_buffers)
            {
                unsigned set = shaderCompiled.get_decoration(resource.id, spv::DecorationDescriptorSet);
                unsigned binding = shaderCompiled.get_decoration(resource.id, spv::DecorationBinding);

                printf("Uniform buffer %s at set = %u binding = %u\n", shaderCompiled.get_name(resource.id).c_str(), set, binding);

                const SPIRV_CROSS_NAMESPACE::SPIRType& basetype = shaderCompiled.get_type(resource.base_type_id);

                printf("\tUBO Size : %d", shaderCompiled.get_declared_struct_size(basetype));
                printMembers(basetype, &shaderCompiled);
            }

            for (auto& resource : shaderResources.storage_buffers)
            {
                unsigned set = shaderCompiled.get_decoration(resource.id, spv::DecorationDescriptorSet);
                unsigned binding = shaderCompiled.get_decoration(resource.id, spv::DecorationBinding);

                printf("Storage buffer %s at set = %u binding = %u\n", shaderCompiled.get_name(resource.id).c_str(), set, binding);

                const SPIRV_CROSS_NAMESPACE::SPIRType& basetype = shaderCompiled.get_type(resource.base_type_id);
                printMembers(basetype, &shaderCompiled);
            }

            if (shaderResources.push_constant_buffers.size() == 1)
            {
                auto& resource = shaderResources.push_constant_buffers[0];
                printf("Push constant %s\n", shaderCompiled.get_name(resource.id).c_str());

                const SPIRV_CROSS_NAMESPACE::SPIRType& basetype = shaderCompiled.get_type(resource.base_type_id);
                printMembers(basetype, &shaderCompiled);
            }

            const auto& specConsts = shaderCompiled.get_specialization_constants();
            for (const auto& consts : specConsts)
            {
                printf("Specialization constant %s ID : %d at constant_id %d\n", shaderCompiled.get_name(consts.id).c_str(), uint32_t(consts.id) , consts.constant_id);
                const SPIRV_CROSS_NAMESPACE::SPIRConstant& constantRef = shaderCompiled.get_constant(consts.id);
                const SPIRV_CROSS_NAMESPACE::SPIRType& typeRef = shaderCompiled.get_type(constantRef.constant_type);
                if (typeRef.basetype == SPIRV_CROSS_NAMESPACE::SPIRType::BaseType::Int)
                {
                    printf("\tConstant default value : %d\n", constantRef.scalar_i32());
                }
                else
                {
                    printf("\tConstant default value : %f\n", constantRef.scalar_f32());
                }
            }

            // Compile to GLSL, ready to give to GL driver.
            std::string source = shaderCompiled.compile();
        }
    }
#endif
    return 0;
}
/*!
 * \file ShaderReflectionProcessor.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once
#include <map>
#include <string>
#include <vector>

#include "../SpirV/spirv.hpp"
#include "ShaderReflected.h"

namespace spirv_cross
{
class Compiler;
}

class ShaderReflectionProcessor
{
private:
    friend class PipelineShaderStageProcessor;

    std::string shaderPath;
    std::string shaderFileName;
    std::vector<uint32_t> shaderCode;
    spirv_cross::Compiler *compiledData;

    ShaderCodeView codeView;

    ShaderStageDescription getStageDesc() const;
    void injectShaderCode(std::vector<uint32_t> &codeCollector) const;
    void setCodeView(uint32_t startIndex, uint32_t size);

public:
    ShaderReflectionProcessor(std::string shaderFilePath);
    ShaderReflectionProcessor(const std::vector<uint32_t> &code, const ShaderCodeView &view);
    ~ShaderReflectionProcessor();

public:
    static uint32_t engineStage(spv::ExecutionModel spirvStage);
    static uint32_t pipelineBindPoint(spv::ExecutionModel spirvStage);
    static uint32_t pipelineStageFlag(spv::ExecutionModel spirvStage);
    static uint32_t shaderStageFlag(spv::ExecutionModel spirvStage);
    static uint8_t readWriteQualifier(bool read, bool write);
    static uint32_t imageViewType(spv::Dim spirvDim, bool bIsArray);
    static TexelComponentFormat texelFormat(spv::ImageFormat format);

    static constexpr uint32_t COMPUTE_STAGE = 0;
    static constexpr uint32_t VERTEX_STAGE = 1;
    static constexpr uint32_t TESS_CONTROL_STAGE = 2;
    static constexpr uint32_t TESS_EVAL_STAGE = 3;
    static constexpr uint32_t GEOMETRY_STAGE = 4;
    static constexpr uint32_t FRAGMENT_STAGE = 5;
    static constexpr uint32_t INVALID = 0x7FFFFFFF;
};

class PipelineShaderStageProcessor
{
private:
    std::vector<ShaderReflectionProcessor *> shaderStages;
    std::string reflectionFile;
    std::string shaderFile;

    ShaderReflected reflectedData;
    std::vector<uint32_t> allShaderCodes;

    void processStages(std::vector<std::map<uint32_t, uint32_t>> &specConstsMaps);
    void processPipelineIO();
    void processDescriptorsSets(const std::vector<std::map<uint32_t, uint32_t>> &specConstsMaps);
    void processPushConstants(const std::vector<std::map<uint32_t, uint32_t>> &specConstsMaps);
    void writeMergedShader();

public:
    PipelineShaderStageProcessor(const std::vector<ShaderReflectionProcessor *> &shaderReflections,
        std::string refFilePath, std::string shaderFilePath);

    void processReflections();
    void writeOutput();
    bool crossCheckWrittenData();
};
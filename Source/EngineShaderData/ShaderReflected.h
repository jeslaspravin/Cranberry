#pragma once

#include <vector>

#include "ShaderDataTypes.h"

struct ShaderCodeView
{
    uint32_t startIdx;
    uint32_t size;
};

struct ShaderStageDescription
{
    uint32_t stage;
    uint32_t pipelineBindPoint;
    std::string entryPoint;
    ShaderCodeView codeView;
    std::vector<ReflectSpecializationConstant> stageSpecializationEntries;
};

struct ShaderReflected
{
    std::vector<ShaderStageDescription> stages;
    std::vector<ReflectInputOutput> inputs;
    std::vector<ReflectInputOutput> outputs;
    std::vector<ReflectDescriptorBody> descriptorsSets;
    ReflectPushConstant pushConstants;
};
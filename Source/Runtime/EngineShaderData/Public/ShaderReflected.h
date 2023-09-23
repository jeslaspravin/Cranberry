/*!
 * \file ShaderReflected.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

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
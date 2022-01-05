/*!
 * \file RenderScene.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "Math/Matrix4.h"
#include "RenderInterface/ShaderCore/ShaderParameters.h"

#include <map>

struct ViewData
{
    Matrix4 view;
    Matrix4 invView;
    Matrix4 projection;
    Matrix4 invProjection;
};

class ENGINERENDERER_EXPORT RenderSceneBase
{
public:
    static const std::map<String, ShaderBufferParamInfo*>& sceneViewParamInfo();
    static void sceneViewSpecConsts(std::map<String, struct SpecializationConstantEntry>& specializationConst);
};
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
#include "RenderInterface/Resources/ShaderResources.h"

struct ViewData
{
    Matrix4 view;
    Matrix4 invView;
    Matrix4 projection;
    Matrix4 invProjection;
};

// TODO(Jeslas) : Rename this file and class to RenderSceneData as I am using it only for view param info as view specialization constants
class ENGINERENDERER_EXPORT RenderSceneBase
{
public:
    constexpr static const TChar *VIEW_PARAM_NAME = TCHAR("viewData");

    static const std::map<StringID, ShaderBufferParamInfo *> &sceneViewParamInfo();
    static void sceneViewSpecConsts(SpecConstantNamedMap &specializationConst);
};
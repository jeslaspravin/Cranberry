#pragma once

#include "../../Core/Math/Matrix4.h"
#include "../../RenderInterface/ShaderCore/ShaderParameters.h"

#include <map>

struct ViewData
{
    Matrix4 view;
    Matrix4 invView;
    Matrix4 projection;
    Matrix4 invProjection;
};

class RenderSceneBase
{
public:
    static const std::map<String, ShaderBufferParamInfo*>& sceneViewParamInfo();
};
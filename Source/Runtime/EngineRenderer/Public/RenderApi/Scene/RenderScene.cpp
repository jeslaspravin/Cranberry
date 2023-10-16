/*!
 * \file RenderScene.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "RenderApi/Scene/RenderScene.h"

BEGIN_BUFFER_DEFINITION(ViewData)
ADD_BUFFER_TYPED_FIELD(view)
ADD_BUFFER_TYPED_FIELD(invView)
ADD_BUFFER_TYPED_FIELD(projection)
ADD_BUFFER_TYPED_FIELD(invProjection)
ADD_BUFFER_TYPED_FIELD(w2clip)
END_BUFFER_DEFINITION();

const std::map<StringID, ShaderBufferParamInfo *> &RenderSceneBase::sceneViewParamInfo()
{
    static ViewDataBufferParamInfo VIEW_DATA_INFO;
    static const std::map<StringID, ShaderBufferParamInfo *> VIEW_PARAMS_INFO{
        {VIEW_PARAM_NAME, &VIEW_DATA_INFO}
    };

    return VIEW_PARAMS_INFO;
}

void RenderSceneBase::sceneViewSpecConsts(SpecConstantNamedMap & /*specializationConst*/) {}

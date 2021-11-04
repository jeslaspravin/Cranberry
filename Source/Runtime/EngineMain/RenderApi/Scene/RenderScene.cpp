#include "RenderScene.h"

BEGIN_BUFFER_DEFINITION(ViewData)
ADD_BUFFER_TYPED_FIELD(view)
ADD_BUFFER_TYPED_FIELD(invView)
ADD_BUFFER_TYPED_FIELD(projection)
ADD_BUFFER_TYPED_FIELD(invProjection)
END_BUFFER_DEFINITION();

const std::map<String, ShaderBufferParamInfo*>& RenderSceneBase::sceneViewParamInfo()
{
    static ViewDataBufferParamInfo VIEW_DATA_INFO;
    static const std::map<String, ShaderBufferParamInfo*> VIEW_PARAMS_INFO
    {
        { "viewData", &VIEW_DATA_INFO }
    };

    return VIEW_PARAMS_INFO;
}

void RenderSceneBase::sceneViewSpecConsts(std::map<String, struct SpecializationConstantEntry>& specializationConst)
{

}


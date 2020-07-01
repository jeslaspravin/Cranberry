#pragma once
#include "../Base/DrawMeshShader.h"


// TODO(Jeslas) : Change this to proper shader from test shader
class StaticMeshUnlit : public DrawMeshShader
{
    DECLARE_GRAPHICS_RESOURCE(StaticMeshUnlit, , DrawMeshShader, );
private:
    StaticMeshUnlit();
};
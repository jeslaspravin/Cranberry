#pragma once
#include "../../../RenderInterface/Rendering/RenderingContexts.h"

class VulkanGlobalRenderingContext : public GlobalRenderingContextBase
{

protected:
	void initApiFactories() override;

};


namespace GraphicsTypes 
{
    typedef VulkanGlobalRenderingContext GlobalRenderingContext;
}
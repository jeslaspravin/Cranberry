#pragma once
#include "../../../Core/String/String.h"
#include "../GraphicsResources.h"
#include "../../CoreGraphicsTypes.h"

#include <glm/detail/type_vec3.hpp>
#include <glm/ext/vector_float2.hpp>

class SamplerInterface : public GraphicsResource
{
    DECLARE_GRAPHICS_RESOURCE(SamplerInterface,,GraphicsResource,)

protected:

    ESamplerFiltering::Type filtering;
    ESamplerFiltering::Type mipFiltering;

    glm::tvec3<ESamplerTilingMode::Type> tilingMode;
    glm::vec2 mipLodRange;

    CoreGraphicsTypes::ECompareOp::Type compareOp;
    uint8 useCompareOp : 1;

    uint8 transparentBorder : 1;
    uint8 intBorder : 1;
    uint8 whiteBorder : 1;

    String resourceName;

protected:
    SamplerInterface();
public:
    SamplerInterface(ESamplerTilingMode::Type samplerTiling, ESamplerFiltering::Type samplerFiltering, float poorMipLod = 0);

    void setMipLod(const float& fineMipLod, const float& poorMipLod);
    void getMipLod(float& fineMipLod, float& poorMipLod);

    void setMipFiltering(ESamplerFiltering::Type samplerFiltering);
    ESamplerFiltering::Type getMipFiltering();
    ESamplerFiltering::Type getFinestFiltering();

    void setCompareOp(bool enable, CoreGraphicsTypes::ECompareOp::Type compareOpValue);
    bool getCompareOp(CoreGraphicsTypes::ECompareOp::Type& compareOpValue);

    void setBorderColor(bool transparent, bool intValue, bool useWhiteColor);
    void getBorderColor(bool& transparent, bool& intValue, bool& useWhiteColor);

    void setTilingMode(ESamplerTilingMode::Type u, ESamplerTilingMode::Type v, ESamplerTilingMode::Type w);
    void getTilingMode(ESamplerTilingMode::Type& u, ESamplerTilingMode::Type& v, ESamplerTilingMode::Type& w);

    /* Graphics Resource overrides */
    String getResourceName() const override;
    void setResourceName(const String& name) override;
    /* End overrides */
};
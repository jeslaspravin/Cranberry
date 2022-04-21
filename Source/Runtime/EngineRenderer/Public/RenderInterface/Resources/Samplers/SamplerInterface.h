/*!
 * \file SamplerInterface.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once
#include <atomic>

#include "Math/Box.h"
#include "RenderInterface/CoreGraphicsTypes.h"
#include "RenderInterface/Resources/GraphicsResources.h"
#include "String/String.h"
#include "Types/Containers/ReferenceCountPtr.h"

struct SamplerCreateInfo
{
    ESamplerFiltering::Type filtering{ ESamplerFiltering::Nearest };
    ESamplerFiltering::Type mipFiltering{ ESamplerFiltering::Nearest };
    std::tuple<ESamplerTilingMode::Type, ESamplerTilingMode::Type, ESamplerTilingMode::Type> tilingMode;
    ValueRange<float> mipLodRange;
    CoreGraphicsTypes::ECompareOp::Type compareOp{ CoreGraphicsTypes::ECompareOp::Greater };
    uint8 useCompareOp;
    uint8 borderColorFlags;
    String resourceName;
};

class ENGINERENDERER_EXPORT SamplerInterface : public GraphicsResource
{
    DECLARE_GRAPHICS_RESOURCE(SamplerInterface, , GraphicsResource, )

private:
    std::atomic<uint32> refCounter;

protected:
    SamplerCreateInfo config;

protected:
    SamplerInterface() = default;

public:
    SamplerInterface(SamplerCreateInfo samplerCI);

    void setMipLod(const float &fineMipLod, const float &poorMipLod);
    void getMipLod(float &fineMipLod, float &poorMipLod);

    void setMipFiltering(ESamplerFiltering::Type samplerFiltering);
    ESamplerFiltering::Type getMipFiltering();
    ESamplerFiltering::Type getFinestFiltering();

    void setCompareOp(bool enable, CoreGraphicsTypes::ECompareOp::Type compareOpValue);
    bool getCompareOp(CoreGraphicsTypes::ECompareOp::Type &compareOpValue);

    void setBorderColor(uint8 samplerBorderColFlags);
    uint8 getBorderColorFlags(bool &transparent, bool &intValue, bool &useWhiteColor) { return config.borderColorFlags; }

    void setTilingMode(ESamplerTilingMode::Type u, ESamplerTilingMode::Type v, ESamplerTilingMode::Type w);
    void getTilingMode(ESamplerTilingMode::Type &u, ESamplerTilingMode::Type &v, ESamplerTilingMode::Type &w);

    /* ReferenceCountPtr implementation */
    void addRef();
    void removeRef();
    uint32 refCount() const;
    /* Graphics Resource overrides */
    String getResourceName() const override;
    void setResourceName(const String &name) override;
    /* End overrides */
};
using SamplerRef = ReferenceCountPtr<SamplerInterface>;
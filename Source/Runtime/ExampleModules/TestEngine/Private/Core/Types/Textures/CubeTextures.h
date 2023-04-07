/*!
 * \file CubeTextures.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "RenderApi/ResourcesInterface/IRenderResource.h"
#include "TexturesBase.h"
#include "Types/Colors.h"

enum class ECubeTextureFormat
{
    CT_NormalizedUI8,
    CT_F32,
    CT_F16
};

struct CubeTextureCreateParams : public TextureBaseCreateParams
{
    UInt2 textureSize;
    ECubeTextureFormat dataFormat;
    // If greater than acceptable it will be clamped, if 0 mips get auto calculated from size
    uint32 mipCount = 0;
};

class CubeTexture
    : public TextureBase
    , public IRenderMemoryResource
{
private:
private:
    static void init(CubeTexture *texture);
    static void destroy(CubeTexture *texture);

protected:
    CubeTexture() = default;
    ~CubeTexture() = default;

    void reinitResources() override;

    static EPixelDataFormat::Type determineDataFormat(ECubeTextureFormat dataFormat);

public:
    uint32 getMipCount() const;

    static CubeTexture *createTexture(const CubeTextureCreateParams &createParams);
    static void destroyTexture(CubeTexture *cubeTexture);

    /* IRenderMemoryResource overrides */
    ReferenceCountPtr<MemoryResource> renderResource() const final { return { textureResource }; }
    /* Overrides end */
};

//////////////////////////////////////////////////////////////////////////
/// Cube texture Read/Write
//////////////////////////////////////////////////////////////////////////

struct CubeTextureRWCreateParams : public CubeTextureCreateParams
{
    bool bWriteOnly = false;
};

class CubeTextureRW : public CubeTexture
{
private:
    bool bWriteOnly = false;

private:
    static void init(CubeTextureRW *texture);

protected:
    CubeTextureRW() = default;
    ~CubeTextureRW() = default;

    void reinitResources() override;

public:
    static CubeTextureRW *createTexture(const CubeTextureRWCreateParams &createParams);
    static void destroyTexture(CubeTextureRW *cubeTexture);
};
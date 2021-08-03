#pragma once

#include "TexturesBase.h"
#include "../Colors.h"

enum class ECubeTextureFormat
{
    CT_F32,
    CT_F16
};

struct CubeTextureCreateParams : public TextureBaseCreateParams
{
    Size2D textureSize;
    ECubeTextureFormat dataFormat;
    // If greater than acceptable it will be clamped, if 0 mips get auto calculated from size
    uint32 mipCount = 0;
};

class CubeTexture : public TextureBase
{
private:
private:
    static void init(CubeTexture* texture);
    static void destroy(CubeTexture* texture);
protected:
    CubeTexture() = default;
    ~CubeTexture() = default;

    void reinitResources() override;

    static EPixelDataFormat::Type determineDataFormat(ECubeTextureFormat dataFormat);
public:
    uint32 getMipCount() const;

    static CubeTexture* createTexture(const CubeTextureCreateParams& createParams);
    static void destroyTexture(CubeTexture* cubeTexture);
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
    static void init(CubeTextureRW* texture);
protected:
    CubeTextureRW() = default;
    ~CubeTextureRW() = default;

    void reinitResources() override;
public:
    static CubeTextureRW* createTexture(const CubeTextureRWCreateParams& createParams);
    static void destroyTexture(CubeTextureRW* cubeTexture);
};
/*!
 * \file ShaderParameterResources.h
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
#include <map>
#include <optional>
#include <set>

#include "Logger/Logger.h"
#include "Reflections/Functions.h"
#include "Types/Containers/ReferenceCountPtr.h"
#include "Types/Containers/ArrayView.h"
#include "RenderInterface/Resources/GraphicsResources.h"
#include "RenderInterface/Resources/MemoryResources.h"
#include "RenderInterface/Resources/Samplers/SamplerInterface.h"
#include "RenderInterface/ShaderCore/ShaderParameters.h"
#include "ShaderDataTypes.h"

class ShaderResource;
class IRenderCommandList;
class IGraphicsInstance;
class Vector2D;
class Vector4D;
class Matrix4;

/**
 * ShaderDescriptorParamType - Wrapper for param type of an descriptors entry of any set. Currently it
 * can be either a texture or buffer or sampler
 *
 * @author Jeslas Pravin
 *
 * @date June 2020
 */
struct ShaderDescriptorParamType
{
protected:
    enum Type
    {
        Texture,
        Buffer,
        Sampler
    };

    Type paramType;

public:
    virtual ~ShaderDescriptorParamType() = default;

    static void wrapReflectedDescriptors(
        std::map<StringID, ShaderDescriptorParamType *> &descriptorParams, const ReflectDescriptorBody &reflectDescriptors,
        std::map<StringID, struct ShaderBufferDescriptorType *> *filterBufferDescriptors = nullptr
    );

    template <typename DescriptorParamType>
    friend const DescriptorParamType *Cast(const ShaderDescriptorParamType *shaderDescriptorType);
    template <typename DescriptorParamType>
    friend DescriptorParamType *Cast(ShaderDescriptorParamType *shaderDescriptorType);
};

struct ShaderTextureDescriptorType : public ShaderDescriptorParamType
{
public:
    constexpr static Type PARAM_TYPE = ShaderDescriptorParamType::Texture;

    ShaderTextureDescriptorType();

    const DescEntryTexture *textureEntryPtr;

    // This usage is just to show that this is image and not texture, For actual usage in shader whether
    // read or write use readWriteState inside textureEntryPtr If sampling or storing
    EImageShaderUsage::Type imageUsageFlags;
    bool bIsAttachedSampler;
};

struct ShaderBufferDescriptorType : public ShaderDescriptorParamType
{
public:
    constexpr static Type PARAM_TYPE = ShaderDescriptorParamType::Buffer;

    ShaderBufferDescriptorType();

    // Buffer parameters info that specify the internal structure of buffer this will be filled with
    // offset and informations from reflection
    ShaderBufferParamInfo *bufferParamInfo;
    const DescEntryBuffer *bufferEntryPtr = nullptr;

    const DescEntryTexelBuffer *texelBufferEntryPtr = nullptr;

    // This storage is just to show that this is buffer and not uniform, For actual usage in shader
    // whether read or write use readWriteState inside bufferEntryPtr
    bool bIsStorage;
};

struct ShaderSamplerDescriptorType : public ShaderDescriptorParamType
{
public:
    constexpr static Type PARAM_TYPE = ShaderDescriptorParamType::Sampler;

    ShaderSamplerDescriptorType();

    const DescEntrySampler *samplerEntryPtr;
};

// Contains shader's API specific layout informations for a descriptors set, Push constants informations
// Assumption :
//  Descriptors set 0 will be common for scene so only one layout will be available in global context
//  Descriptors set 1 will be common for a vertex type instance so layout will be available in vertex
//  specific type objects Descriptors set 2 will be unique for each shader
class ENGINERENDERER_EXPORT ShaderSetParametersLayout : public GraphicsResource
{
    DECLARE_GRAPHICS_RESOURCE(ShaderSetParametersLayout, , GraphicsResource, )

protected:
    // Since 2nd set is only of interest for this object
    uint32 shaderSetID;

    const ShaderResource *respectiveShaderRes;
    std::map<StringID, ShaderDescriptorParamType *> paramsLayout;
    bool bHasBindless;

protected:
    ShaderSetParametersLayout() = default;
    ShaderSetParametersLayout(const ShaderResource *shaderResource, uint32 setID);

    // Bind buffer info so it can be filled with offset, stride and size informations
    virtual void bindBufferParamInfo(std::map<StringID, struct ShaderBufferDescriptorType *> &) const {};

public:
    /* GraphicsResource overrides */
    virtual void init() override;
    virtual void release() override;
    /* Override ends */

    const ShaderDescriptorParamType *parameterDescription(uint32 &outSetIdx, StringID paramName) const;
    const ShaderDescriptorParamType *parameterDescription(StringID paramName) const;
    const std::map<StringID, ShaderDescriptorParamType *> &allParameterDescriptions() const;
    uint32 getSetID() const { return shaderSetID; }
    bool hasBindless() const { return bHasBindless; }
    const ShaderResource *getShaderResource() const { return respectiveShaderRes; }
};

// Contains shader's all descriptors set layouts
class ENGINERENDERER_EXPORT ShaderParametersLayout : public GraphicsResource
{
    DECLARE_GRAPHICS_RESOURCE(ShaderParametersLayout, , GraphicsResource, )

protected:
    const ShaderResource *respectiveShaderRes;
    /*
     * Assumed that all descriptors in the shader has unique name irrespective of set just like vertex
     * attributes uint32 in pair is descriptor set Index
     */
    std::map<StringID, std::pair<uint32, ShaderDescriptorParamType *>> paramsLayout;

protected:
    ShaderParametersLayout() = default;
    ShaderParametersLayout(const ShaderResource *shaderResource);

public:
    /* GraphicsResource overrides */
    virtual void init() override;
    virtual void release() override;
    /* Override ends */

    virtual bool hasBindless(uint32) const { return false; }

    const ShaderDescriptorParamType *parameterDescription(uint32 &outSetIdx, StringID paramName) const;
    const ShaderDescriptorParamType *parameterDescription(StringID paramName) const;
    std::map<StringID, ShaderDescriptorParamType *> allParameterDescriptions() const;
    uint32 getSetID(StringID paramName) const;
    const ShaderResource *getShaderResource() const { return respectiveShaderRes; }
};

class ENGINERENDERER_EXPORT ShaderParameters : public GraphicsResource
{
    DECLARE_GRAPHICS_RESOURCE(ShaderParameters, , GraphicsResource, )

private:
    std::atomic<uint32> refCounter;

protected:
    struct ParamUpdateLambdaOut
    {
        std::vector<struct BatchCopyBufferData> *bufferUpdates;
    };

    using ParamUpdateLambda = LambdaFunction<void, ParamUpdateLambdaOut &, IRenderCommandList *, IGraphicsInstance *>;
    struct BufferParameterUpdate
    {
        StringID bufferName;
        StringID paramName;
        uint32 index = 0;

        bool operator== (const BufferParameterUpdate &other) const
        {
            return bufferName == other.bufferName && paramName == other.paramName && index == other.index;
        }
        struct Hasher
        {
            _NODISCARD size_t operator() (const BufferParameterUpdate &keyVal) const noexcept;
        };
    };

    struct BufferParametersData
    {
        struct BufferParameter
        {
            // Always will be 0th element of outer buffer's array
            void *outerPtr = nullptr;
            StringID outerName;
            const ShaderBufferField *bufferField = nullptr;
        };
        struct RuntimeArrayParameter
        {
            StringID paramName;
            uint32 offset;
            // Current number of supported elements
            uint32 currentCount;
            std::vector<uint8> runtimeArrayCpuBuffer;
        };

        // If the buffer resource is set manually, then don't manage it here
        bool bIsExternal = false;
        const ShaderBufferDescriptorType *descriptorInfo;
        uint8 *cpuBuffer;
        BufferResourceRef gpuBuffer;
        // Linear list of all level parameters inside this buffer
        std::map<StringID, BufferParameter> bufferParams;
        std::optional<RuntimeArrayParameter> runtimeArray;
    };

    struct TexelParameterData
    {
        const ShaderBufferDescriptorType *descriptorInfo;
        std::vector<BufferResourceRef> gpuBuffers;
    };

    struct TextureParameterData
    {
        struct TextureViewAndSampler
        {
            ImageResourceRef texture;
            ImageViewInfo viewInfo;
            // Optional sampler
            SamplerRef sampler;
        };
        const ShaderTextureDescriptorType *descriptorInfo;

        std::vector<TextureViewAndSampler> textures;
    };

    struct SamplerParameterData
    {
        const ShaderSamplerDescriptorType *descriptorInfo;

        std::vector<SamplerRef> samplers;
    };

    std::map<StringID, BufferParametersData> shaderBuffers;
    std::map<StringID, TexelParameterData> shaderTexels;
    std::map<StringID, TextureParameterData> shaderTextures;
    std::map<StringID, SamplerParameterData> shaderSamplers;
    // will be used only in terms of ShaderParametersLayout
    std::set<uint32> ignoredSets;

    std::vector<BufferParameterUpdate> bufferUpdates;
    std::set<StringID> bufferResourceUpdates;
    std::set<std::pair<StringID, uint32>> texelUpdates;
    std::set<std::pair<StringID, uint32>> textureUpdates;
    std::set<std::pair<StringID, uint32>> samplerUpdates;
    std::vector<ParamUpdateLambda> genericUpdates;

    const GraphicsResource *paramLayout;
    String descriptorSetName;

public:
    /* ReferenceCountPtr implementation */
    void addRef();
    void removeRef();
    uint32 refCount() const;
    /* GraphicsResource overrides */
    virtual void init() override;
    virtual void release() override;
    String getResourceName() const final;
    void setResourceName(const String &name) final;
    /* Override ends */

    const GraphicsResource *getParamLayout() const { return paramLayout; }

    // Read only
    std::vector<std::pair<ImageResourceRef, const ShaderTextureDescriptorType *>> getAllReadOnlyTextures() const;
    std::vector<std::pair<BufferResourceRef, const ShaderBufferDescriptorType *>> getAllReadOnlyBuffers() const;
    std::vector<std::pair<BufferResourceRef, const ShaderBufferDescriptorType *>> getAllReadOnlyTexels() const;
    // Read Write and Write
    std::vector<std::pair<ImageResourceRef, const ShaderTextureDescriptorType *>> getAllWriteTextures() const;
    std::vector<std::pair<BufferResourceRef, const ShaderBufferDescriptorType *>> getAllWriteBuffers() const;
    std::vector<std::pair<BufferResourceRef, const ShaderBufferDescriptorType *>> getAllWriteTexels() const;

    virtual void updateParams(IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance);
    void pullBufferParamUpdates(std::vector<BatchCopyBufferData> &copies, IRenderCommandList *cmdList, IGraphicsInstance *graphicsInstance);

    // Below set*Param can be used to set parameters directly inside a buffer. It cannot set level deeper than that
    bool setIntParam(StringID paramName, int32 value, uint32 index = 0);
    bool setIntParam(StringID paramName, uint32 value, uint32 index = 0);
    bool setFloatParam(StringID paramName, float value, uint32 index = 0);
    bool setVector2Param(StringID paramName, const Vector2D &value, uint32 index = 0);
    bool setVector4Param(StringID paramName, const Vector4D &value, uint32 index = 0);
    bool setMatrixParam(StringID paramName, const Matrix4 &value, uint32 index = 0);
    bool setIntParam(StringID paramName, StringID bufferName, int32 value, uint32 index = 0);
    bool setIntParam(StringID paramName, StringID bufferName, uint32 value, uint32 index = 0);
    bool setFloatParam(StringID paramName, StringID bufferName, float value, uint32 index = 0);
    bool setVector2Param(StringID paramName, StringID bufferName, const Vector2D &value, uint32 index = 0);
    bool setVector4Param(StringID paramName, StringID bufferName, const Vector4D &value, uint32 index = 0);
    bool setMatrixParam(StringID paramName, StringID bufferName, const Matrix4 &value, uint32 index = 0);
    template <typename BufferType>
    bool setBuffer(StringID paramName, const BufferType &bufferValue, uint32 index = 0);
    bool setBuffer(StringID paramName, const void *bufferValue, uint32 index = 0);
    // Below set*AtPath can be used to set parameters in buffer deeper than above sets can.
    // 0th must be bound buffer's name, (n-1)th must be the param name to set
    bool setIntAtPath(ArrayView<const StringID> pathNames, ArrayView<const uint32> indices, int32 value);
    bool setIntAtPath(ArrayView<const StringID> pathNames, ArrayView<const uint32> indices, uint32 value);
    bool setFloatAtPath(ArrayView<const StringID> pathNames, ArrayView<const uint32> indices, float value);
    bool setVector2AtPath(ArrayView<const StringID> pathNames, ArrayView<const uint32> indices, const Vector2D &value);
    bool setVector4AtPath(ArrayView<const StringID> pathNames, ArrayView<const uint32> indices, const Vector4D &value);
    bool setMatrixAtPath(ArrayView<const StringID> pathNames, ArrayView<const uint32> indices, const Matrix4 &value);

    bool setTexelParam(StringID paramName, BufferResourceRef texelBuffer, uint32 index = 0);
    bool setTextureParam(StringID paramName, ImageResourceRef texture, uint32 index = 0);
    bool setTextureParam(StringID paramName, ImageResourceRef texture, SamplerRef sampler, uint32 index = 0);
    bool setTextureParamViewInfo(StringID paramName, const ImageViewInfo &textureViewInfo, uint32 index = 0);
    bool setSamplerParam(StringID paramName, SamplerRef sampler, uint32 index = 0);

    // Below get*Param can be used to get parameters directly inside a buffer. It cannot get level deeper than that
    int32 getIntParam(StringID paramName, uint32 index = 0) const;
    uint32 getUintParam(StringID paramName, uint32 index = 0) const;
    float getFloatParam(StringID paramName, uint32 index = 0) const;
    Vector2D getVector2Param(StringID paramName, uint32 index = 0) const;
    Vector4D getVector4Param(StringID paramName, uint32 index = 0) const;
    Matrix4 getMatrixParam(StringID paramName, uint32 index = 0) const;
    int32 getIntParam(StringID paramName, StringID bufferName, uint32 index = 0) const;
    uint32 getUintParam(StringID paramName, StringID bufferName, uint32 index = 0) const;
    float getFloatParam(StringID paramName, StringID bufferName, uint32 index = 0) const;
    Vector2D getVector2Param(StringID paramName, StringID bufferName, uint32 index = 0) const;
    Vector4D getVector4Param(StringID paramName, StringID bufferName, uint32 index = 0) const;
    Matrix4 getMatrixParam(StringID paramName, StringID bufferName, uint32 index = 0) const;
    // Below get*AtPath can be used to get parameters in buffer deeper than above gets can.
    // 0th must be bound buffer's name, (n-1)th must be the param name to get
    int32 getIntAtPath(ArrayView<const StringID> pathNames, ArrayView<const uint32> indices) const;
    uint32 getUintAtPath(ArrayView<const StringID> pathNames, ArrayView<const uint32> indices) const;
    float getFloatAtPath(ArrayView<const StringID> pathNames, ArrayView<const uint32> indices) const;
    Vector2D getVector2AtPath(ArrayView<const StringID> pathNames, ArrayView<const uint32> indices) const;
    Vector4D getVector4AtPath(ArrayView<const StringID> pathNames, ArrayView<const uint32> indices) const;
    Matrix4 getMatrixAtPath(ArrayView<const StringID> pathNames, ArrayView<const uint32> indices) const;

    BufferResourceRef getTexelParam(StringID paramName, uint32 index = 0) const;
    ImageResourceRef getTextureParam(StringID paramName, uint32 index = 0) const;
    ImageResourceRef getTextureParam(SamplerRef &outSampler, StringID paramName, uint32 index = 0) const;
    SamplerRef getSamplerParam(StringID paramName, uint32 index = 0) const;

    bool setBufferResource(StringID bufferName, BufferResourceRef buffer);
    BufferResourceRef getBufferResource(StringID bufferName);
    uint32 getBufferRequiredSize(StringID bufferName) const;
    // Resizing must be done at external wrapper buffer name
    void resizeRuntimeBuffer(StringID bufferName, uint32 minCount);
    uint32 getRuntimeBufferRequiredSize(StringID bufferName, uint32 count) const;
    uint32 getRuntimeBufferGpuStride(StringID bufferName) const;
    uint32 getRuntimeBufferCount(StringID bufferName) const;

private:
    void initBufferParams(
        BufferParametersData &bufferParamData, const ShaderBufferParamInfo *bufferParamInfo, void *outerPtr, StringID outerName
    ) const;
    // Runtime array related, returns true if runtime array is setup
    bool initRuntimeArrayData(BufferParametersData &bufferParamData) const;

    // Init entire param maps uniforms, textures, samplers, images, buffers
    void initParamsMaps(
        const std::map<StringID, ShaderDescriptorParamType *> &paramsDesc,
        const std::vector<std::vector<SpecializationConstantEntry>> &specializationConsts
    );
    std::pair<const BufferParametersData *, const BufferParametersData::BufferParameter *>
    findBufferParam(StringID &bufferName, StringID paramName) const;
    void *getOuterPtrForPath(
        std::vector<const BufferParametersData::BufferParameter *> &outInnerBufferParams, ArrayView<const StringID> pathNames,
        ArrayView<const uint32> indices
    ) const;
    template <typename FieldType>
    bool setFieldParam(StringID paramName, const FieldType &value, uint32 index);
    template <typename FieldType>
    bool setFieldParam(StringID paramName, StringID bufferName, const FieldType &value, uint32 index);
    template <typename FieldType>
    bool setFieldAtPath(ArrayView<const StringID> pathNames, ArrayView<const uint32> indices, const FieldType &value);
    template <typename FieldType>
    FieldType getFieldParam(StringID paramName, uint32 index) const;
    template <typename FieldType>
    FieldType getFieldParam(StringID paramName, StringID bufferName, uint32 index) const;
    template <typename FieldType>
    FieldType getFieldAtPath(ArrayView<const StringID> pathNames, ArrayView<const uint32> indices) const;

protected:
    ShaderParameters() = default;
    ShaderParameters(const GraphicsResource *shaderParamLayout, const std::set<uint32> &ignoredSetIds = {});
};

using ShaderParametersRef = ReferenceCountPtr<ShaderParameters>;

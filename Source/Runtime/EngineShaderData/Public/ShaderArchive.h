/*!
 * \file ShaderArchive.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once
#include "ShaderReflected.h"

#include <type_traits>

class ShaderArchive
{
private:
    // Always points to next read start point
    uint32_t archivePtr;
    std::vector<unsigned char> archive;
    bool bIsLoading;
    bool status = true;

    bool moveFwd(uint32_t ptCount)
    {
        // If resize is required
        if (archive.size() < archivePtr + ptCount)
        {
            // Resizing cannot be done while loading
            if (!bIsLoading)
            {
                archive.resize(archivePtr + ptCount);
            }
            else
            {
                return false;
            }
        }
        archivePtr += ptCount;
        return true;
    }

public:
    ShaderArchive()
        : archivePtr(0)
        , archive()
        , bIsLoading(false)
    {}

    ShaderArchive(const std::vector<unsigned char> &data)
        : archivePtr(0)
        , archive(data)
        , bIsLoading(true)
    {}
    ShaderArchive(const ShaderArchive &) = delete;
    ShaderArchive(ShaderArchive &&) = delete;

    inline bool isLoading() const { return bIsLoading; }
    const std::vector<unsigned char> &archiveData() const { return archive; }

    // Normal types both integral and custom specialized
    template <typename ArchiveType>
    friend std::enable_if_t<!std::is_pointer_v<ArchiveType>, void> operator<<(ShaderArchive &archive, ArchiveType &typeData);
    // Integral collection - vector type
    template <typename Type>
    friend std::enable_if_t<std::is_integral_v<Type>, void> operator<<(ShaderArchive &archive, std::vector<Type> &typeData);
};

// All specialization declarations
template <typename ArchiveType>
std::enable_if_t<!std::is_pointer_v<ArchiveType>, void> operator<<(ShaderArchive &archive, ArchiveType &typeData);
template <>
void operator<< <std::string>(ShaderArchive &archive, std::string &typeData);
template <>
void operator<< <ShaderStageDescription>(ShaderArchive &archive, ShaderStageDescription &typeData);
template <>
void operator<< <PushConstantEntry>(ShaderArchive &archive, PushConstantEntry &typeData);
template <>
void operator<< <ReflectBufferShaderField>(ShaderArchive &archive, ReflectBufferShaderField &typeData);
template <>
void operator<< <ReflectTexelBufferShaderField>(ShaderArchive &archive, ReflectTexelBufferShaderField &typeData);
template <>
void operator<< <ReflectTextureShaderField>(ShaderArchive &archive, ReflectTextureShaderField &typeData);
template <>
void operator<< <ReflectDescriptorBody>(ShaderArchive &archive, ReflectDescriptorBody &typeData);
template <>
void operator<< <ShaderReflected>(ShaderArchive &archive, ShaderReflected &typeData);

// Integral collection - vector type
template <typename Type>
std::enable_if_t<std::is_integral_v<Type>, void> operator<<(ShaderArchive &archive, std::vector<Type> &typeData);
// Non-Integral collection - vector type
template <typename Type>
std::enable_if_t<!std::is_integral_v<Type>, void> operator<<(ShaderArchive &archive, std::vector<Type> &typeData);

template <typename Type>
void operator<<(ShaderArchive &archive, NamedAttribute<Type> &typeData);
template <typename Type>
void operator<<(ShaderArchive &archive, StructInnerFields<Type> &typeData);
template <typename Type>
void operator<<(ShaderArchive &archive, DescriptorSetEntry<Type> &typeData);

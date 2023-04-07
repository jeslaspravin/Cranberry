/*!
 * \file CommonTypesSerialization.h
 *
 * \author Jeslas
 * \date June 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "Serialization/ArchiveBase.h"
#include "Math/CoreMathTypes.h"
#include "Math/Box.h"
#include "Types/Transform3D.h"
#include "Types/Colors.h"
#include "String/StringID.h"
#include "String/NameString.h"

// serialize glm types
struct SerializeGlmVec
{
    template <SizeT Idx, ArchiveTypeName ArchiveType, glm::length_t Count, typename ElementType, glm::qualifier Qualifier>
    FORCE_INLINE static void serialize(ArchiveType &archive, glm::vec<Count, ElementType, Qualifier> &value)
    {
        archive << value[Idx];
    }
    template <ArchiveTypeName ArchiveType, glm::length_t Count, typename ElementType, glm::qualifier Qualifier, SizeT... Indices>
    static ArchiveType &serialize(ArchiveType &archive, glm::vec<Count, ElementType, Qualifier> &value, std::index_sequence<Indices...>)
    {
        // Expand inline
        (serialize<Indices>(archive, value), ...);
        return archive;
    }
};
template <ArchiveTypeName ArchiveType, glm::length_t Count, typename ElementType, glm::qualifier Qualifier>
ArchiveType &operator<< (ArchiveType &archive, glm::vec<Count, ElementType, Qualifier> &value)
{
    return SerializeGlmVec::serialize(archive, value, std::make_index_sequence<Count>{});
}

template <ArchiveTypeName ArchiveType>
ArchiveType &operator<< (ArchiveType &archive, Vector2 &value)
{
    return archive << value.x() << value.y();
}

template <ArchiveTypeName ArchiveType>
ArchiveType &operator<< (ArchiveType &archive, Vector3 &value)
{
    return archive << value.x() << value.y() << value.z();
}

template <ArchiveTypeName ArchiveType>
ArchiveType &operator<< (ArchiveType &archive, Vector4 &value)
{
    return archive << value.x() << value.y() << value.z() << value.w();
}

template <ArchiveTypeName ArchiveType>
ArchiveType &operator<< (ArchiveType &archive, Matrix2 &value)
{
    archive << value[0] << value[1];
    return archive;
}

template <ArchiveTypeName ArchiveType>
ArchiveType &operator<< (ArchiveType &archive, Matrix3 &value)
{
    archive << value[0] << value[1] << value[2];
    return archive;
}

template <ArchiveTypeName ArchiveType>
ArchiveType &operator<< (ArchiveType &archive, Matrix4 &value)
{
    archive << value[0] << value[1] << value[2] << value[3];
    return archive;
}

template <ArchiveTypeName ArchiveType>
ArchiveType &operator<< (ArchiveType &archive, Quat &value)
{
    return archive << value.x << value.y << value.z << value.w;
}

template <ArchiveTypeName ArchiveType>
ArchiveType &operator<< (ArchiveType &archive, Rotation &value)
{
    return archive << value.roll() << value.pitch() << value.yaw();
}

template <ArchiveTypeName ArchiveType>
ArchiveType &operator<< (ArchiveType &archive, Transform3D &value)
{
    return archive << value.transformTranslation << value.transformRotation << value.transformScale;
}

template <ArchiveTypeName ArchiveType>
ArchiveType &operator<< (ArchiveType &archive, Color &value)
{
    return archive << value.getColorValue().r << value.getColorValue().g << value.getColorValue().b << value.getColorValue().a;
}

template <ArchiveTypeName ArchiveType>
ArchiveType &operator<< (ArchiveType &archive, LinearColor &value)
{
    return archive << value.getColorValue().r << value.getColorValue().g << value.getColorValue().b << value.getColorValue().a;
}

template <ArchiveTypeName ArchiveType>
ArchiveType &operator<< (ArchiveType &archive, StringID &value)
{
    archive << value.id;
#if DEV_BUILD
    // In development build we have to setup debug string as well
    if (archive.isLoading())
    {
        value = StringID(value.id);
    }
#endif
    return archive;
}

template <ArchiveTypeName ArchiveType>
ArchiveType &operator<< (ArchiveType &archive, NameString &value)
{
    archive << value.nameStr;
    if (archive.isLoading())
    {
        value.id = value.nameStr;
    }
    return archive;
}

// Box type
template <ArchiveTypeName ArchiveType, typename ElementType, uint32 ElementCount>
ArchiveType &operator<< (ArchiveType &archive, Box<ElementType, ElementCount> &value)
{
    return archive << value.minBound << value.maxBound;
}
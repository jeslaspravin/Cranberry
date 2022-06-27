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
#include "Types/Transform3D.h"
#include "Types/Colors.h"
#include "String/StringID.h"
#include "String/NameString.h"

template <ArchiveTypeName ArchiveType>
ArchiveType &operator<<(ArchiveType &archive, Vector2D &value)
{
    return archive << value.x() << value.y();
}

template <ArchiveTypeName ArchiveType>
ArchiveType &operator<<(ArchiveType &archive, Vector3D &value)
{
    return archive << value.x() << value.y() << value.z();
}

template <ArchiveTypeName ArchiveType>
ArchiveType &operator<<(ArchiveType &archive, Vector4D &value)
{
    return archive << value.x() << value.y() << value.z() << value.w();
}

template <ArchiveTypeName ArchiveType>
ArchiveType &operator<<(ArchiveType &archive, Matrix2 &value)
{
    archive << value[0].x << value[1].x;
    archive << value[0].y << value[1].y;
    return archive;
}

template <ArchiveTypeName ArchiveType>
ArchiveType &operator<<(ArchiveType &archive, Matrix3 &value)
{
    archive << value[0].x << value[1].x << value[2].x;
    archive << value[0].y << value[1].y << value[2].y;
    archive << value[0].z << value[1].z << value[2].z;
    return archive;
}

template <ArchiveTypeName ArchiveType>
ArchiveType &operator<<(ArchiveType &archive, Matrix4 &value)
{
    archive << value[0].x << value[1].x << value[2].x << value[3].x;
    archive << value[0].y << value[1].y << value[2].y << value[3].y;
    archive << value[0].z << value[1].z << value[2].z << value[3].z;
    archive << value[0].w << value[1].w << value[2].w << value[3].w;
    return archive;
}

template <ArchiveTypeName ArchiveType>
ArchiveType &operator<<(ArchiveType &archive, Quat &value)
{
    return archive << value.x << value.y << value.z << value.w;
}

template <ArchiveTypeName ArchiveType>
ArchiveType &operator<<(ArchiveType &archive, Rotation &value)
{
    return archive << value.roll() << value.pitch() << value.yaw();
}

template <ArchiveTypeName ArchiveType>
ArchiveType &operator<<(ArchiveType &archive, Transform3D &value)
{
    archive << value.transformTranslation << value.transformRotation << value.transformScale;
    if (archive.isLoading())
    {
        value.bCachedLatest = false;
    }
    return archive;
}

template <ArchiveTypeName ArchiveType>
ArchiveType &operator<<(ArchiveType &archive, Color &value)
{
    return archive << value.getColorValue().r << value.getColorValue().g << value.getColorValue().b << value.getColorValue().a;
}

template <ArchiveTypeName ArchiveType>
ArchiveType &operator<<(ArchiveType &archive, LinearColor &value)
{
    return archive << value.getColorValue().r << value.getColorValue().g << value.getColorValue().b << value.getColorValue().a;
}

template <ArchiveTypeName ArchiveType>
ArchiveType &operator<<(ArchiveType &archive, StringID &value)
{
    return archive << value.id;
}

template <ArchiveTypeName ArchiveType>
ArchiveType &operator<<(ArchiveType &archive, NameString &value)
{
    archive << value.nameStr;
    if (archive.isLoading())
    {
        value.id = value.nameStr;
    }
}
/*!
 * \file BinaryArchive.cpp
 *
 * \author Jeslas
 * \date March 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "Serialization/BinaryArchive.h"
#include "Memory/Memory.h"
#include "String/TCharString.h"
#include "Types/Platform/LFS/File/FileHelper.h"
#include "Types/Templates/TemplateTypes.h"

template <typename Type>
FORCE_INLINE void serializeBytesSwapped(Type &data, BinaryArchive &archive, UIntToType<1>)
{
    if (archive.isLoading())
    {
        archive.stream()->read(&data, sizeof(Type));
    }
    else
    {
        archive.stream()->write(&data, sizeof(Type));
    }
}
template <typename Type>
FORCE_INLINE void serializeBytesSwapped(Type &data, BinaryArchive &archive, UIntToType<2>)
{
    if (archive.isLoading())
    {
        uint16 readData;
        archive.stream()->read(&readData, sizeof(Type));
        readData = FileHelper::bytesSwap(readData);
        CBEMemory::memCopy(&data, &readData, sizeof(Type));
    }
    else
    {
        uint16 writeData = FileHelper::bytesSwap(*reinterpret_cast<uint16 *>(&data));
        archive.stream()->write(&writeData, sizeof(Type));
    }
}
template <typename Type>
FORCE_INLINE void serializeBytesSwapped(Type &data, BinaryArchive &archive, UIntToType<4>)
{
    if (archive.isLoading())
    {
        uint32 readData;
        archive.stream()->read(&readData, sizeof(Type));
        readData = FileHelper::bytesSwap(readData);
        CBEMemory::memCopy(&data, &readData, sizeof(Type));
    }
    else
    {
        uint32 writeData = FileHelper::bytesSwap(*reinterpret_cast<uint32 *>(&data));
        archive.stream()->write(&writeData, sizeof(Type));
    }
}
template <typename Type>
FORCE_INLINE void serializeBytesSwapped(Type &data, BinaryArchive &archive, UIntToType<8>)
{
    if (archive.isLoading())
    {
        uint64 readData;
        archive.stream()->read(&readData, sizeof(Type));
        readData = FileHelper::bytesSwap(readData);
        CBEMemory::memCopy(&data, &readData, sizeof(Type));
    }
    else
    {
        uint64 writeData = FileHelper::bytesSwap(*reinterpret_cast<uint64 *>(&data));
        archive.stream()->write(&writeData, sizeof(Type));
    }
}

// Generic implementation
template <typename Type, SizeT TypeSize>
FORCE_INLINE void serializeBytesSwapped(Type &data, BinaryArchive &archive, UIntToType<TypeSize>)
{
    if (archive.isLoading())
    {
        archive.stream()->read(&data, TypeSize);
        FileHelper::bytesSwap(&data, TypeSize);
    }
    else
    {
        Type swappedData = data;
        FileHelper::bytesSwap(&swappedData, TypeSize);
        archive.stream()->write(&swappedData, TypeSize);
    }
}

template <typename Type, SizeT TypeSize = sizeof(Type)>
FORCE_INLINE void serializeBytesSwapped(Type &data, BinaryArchive &archive)
{
    serializeBytesSwapped(data, archive, UIntToType<TypeSize>{});
}

template <typename Type>
FORCE_INLINE void serializeBytesOrdered(Type &data, BinaryArchive &archive)
{
    if (archive.ifSwapBytes())
    {
        serializeBytesSwapped(data, archive);
    }
    else
    {
        // Sending size as 1 byte outputs the data without any swapping
        serializeBytesSwapped<Type, 1>(data, archive);
    }
}

ArchiveBase &BinaryArchive::serialize(TChar *value)
{
    // Up to 512 bytes in stack
    uint8 buffer[512];
    std::pmr::monotonic_buffer_resource memRes(buffer, ARRAY_LENGTH(buffer));

    SizeT len;
    // We always serialize as utf8
    if (isLoading())
    {
        serialize(len);

        std::pmr::string str{ &memRes };
        str.resize(len);
        stream()->read(str.data(), len);

        String outStr{ UTF8_TO_TCHAR(str.c_str()) };
        CBEMemory::memCopy(value, outStr.data(), outStr.length());
    }
    else
    {
        std::pmr::string writeStr{ TCHAR_TO_UTF8(value), &memRes };
        len = writeStr.length();
        serialize(len);
        stream()->write(writeStr.data(), writeStr.length());
    }
    return *this;
}

ArchiveBase &BinaryArchive::serialize(String &value)
{
    // Up to 512 bytes in stack
    uint8 buffer[512];
    std::pmr::monotonic_buffer_resource memRes(buffer, ARRAY_LENGTH(buffer));

    SizeT len;
    // We always serialize as utf8s
    if (isLoading())
    {
        serialize(len);

        std::pmr::string str{ &memRes };
        str.resize(len);
        stream()->read(str.data(), len);

        value = { UTF8_TO_TCHAR(str.c_str()) };
    }
    else
    {
        std::pmr::string writeStr{ TCHAR_TO_UTF8(value.getChar()), &memRes };
        len = writeStr.length();
        serialize(len);
        stream()->write(writeStr.data(), writeStr.length());
    }
    return *this;
}

ArchiveBase &BinaryArchive::serialize(uint8 &value)
{
    serializeBytesOrdered(value, *this);
    return *this;
}

ArchiveBase &BinaryArchive::serialize(uint16 &value)
{
    serializeBytesOrdered(value, *this);
    return *this;
}

ArchiveBase &BinaryArchive::serialize(uint32 &value)
{
    serializeBytesOrdered(value, *this);
    return *this;
}

ArchiveBase &BinaryArchive::serialize(uint64 &value)
{
    serializeBytesOrdered(value, *this);
    return *this;
}

ArchiveBase &BinaryArchive::serialize(int8 &value)
{
    serializeBytesOrdered(value, *this);
    return *this;
}

ArchiveBase &BinaryArchive::serialize(int16 &value)
{
    serializeBytesOrdered(value, *this);
    return *this;
}

ArchiveBase &BinaryArchive::serialize(int32 &value)
{
    serializeBytesOrdered(value, *this);
    return *this;
}

ArchiveBase &BinaryArchive::serialize(int64 &value)
{
    serializeBytesOrdered(value, *this);
    return *this;
}

ArchiveBase &BinaryArchive::serialize(float &value)
{
    serializeBytesOrdered(value, *this);
    return *this;
}

ArchiveBase &BinaryArchive::serialize(double &value)
{
    serializeBytesOrdered(value, *this);
    return *this;
}

ArchiveBase &BinaryArchive::serialize(bool &value)
{
    serializeBytesOrdered(value, *this);
    return *this;
}

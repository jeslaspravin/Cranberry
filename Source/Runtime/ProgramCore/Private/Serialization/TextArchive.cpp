/*!
 * \file TextArchive.cpp
 *
 * \author Jeslas
 * \date March 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "Serialization/TextArchive.h"
#include "Memory/Memory.h"
#include "String/String.h"

#include <charconv>
#include <float.h>
#include <memory_resource>

// Reads null terminated string from stream into buffer pointed by firstChar and returns length of string
SizeT readTextFromStream(AChar *firstChar, ArchiveStream &stream)
{
    SizeT len = 0;
    while (uint8 byteVal = stream.readForwardAt(len))
    {
        firstChar[len] = (AChar)(byteVal);
        len++;
    }
    return len;
}
SizeT lengthFromTextStream(ArchiveStream &stream)
{
    SizeT len = 0;
    while (uint8 byteVal = stream.readForwardAt(len))
    {
        len++;
    }
    return len;
}

template <typename Type>
void serializeAsText(Type &value, AChar *buffer, SizeT bufferLen, TextArchive &archive)
{
    if (archive.isLoading())
    {
        AChar *firstChar = buffer;
        SizeT len = readTextFromStream(firstChar, *archive.stream());

        std::from_chars(firstChar, firstChar + len, value);
        // +1 To include null termination
        archive.stream()->moveForward(len + 1);
    }
    else
    {
        std::to_chars_result result = std::to_chars(buffer, buffer + bufferLen, value);
        // Do not have to + 1 the len as end ptr returned is one after last char
        SizeT len = (result.ptr - buffer);
        buffer[len] = '\0';
        archive.stream()->write(buffer, len + 1);
    }
}

ArchiveBase &TextArchive::serialize(bool &value)
{
    if (isLoading())
    {
        if (stream()->readForwardAt(0) == 't')
        {
            value = true;
            stream()->moveForward(5);
        }
        else
        {
            value = false;
            stream()->moveForward(6);
        }
    }
    else
    {
        if (value)
        {
            stream()->write("true", 5);
        }
        else
        {
            stream()->write("false", 6);
        }
    }
    return *this;
}

// Char buffer length from
// https://stackoverflow.com/questions/1701055/what-is-the-maximum-length-in-chars-needed-to-represent-any-double-value
ArchiveBase &TextArchive::serialize(double &value)
{
    AChar buffer[3 + DBL_MANT_DIG - DBL_MIN_EXP];
    serializeAsText(value, buffer, ARRAY_LENGTH(buffer), *this);
    return *this;
}

ArchiveBase &TextArchive::serialize(float &value)
{
    AChar buffer[3 + FLT_MANT_DIG - FLT_MIN_EXP];
    serializeAsText(value, buffer, ARRAY_LENGTH(buffer), *this);
    return *this;
}

ArchiveBase &TextArchive::serialize(int64 &value)
{
    AChar buffer[21];
    serializeAsText(value, buffer, ARRAY_LENGTH(buffer), *this);
    return *this;
}

ArchiveBase &TextArchive::serialize(uint64 &value)
{
    AChar buffer[21];
    serializeAsText(value, buffer, ARRAY_LENGTH(buffer), *this);
    return *this;
}

ArchiveBase &TextArchive::serialize(int32 &value)
{
    AChar buffer[12];
    serializeAsText(value, buffer, ARRAY_LENGTH(buffer), *this);
    return *this;
}

ArchiveBase &TextArchive::serialize(uint32 &value)
{
    AChar buffer[12];
    serializeAsText(value, buffer, ARRAY_LENGTH(buffer), *this);
    return *this;
}

ArchiveBase &TextArchive::serialize(int16 &value)
{
    AChar buffer[7];
    serializeAsText(value, buffer, ARRAY_LENGTH(buffer), *this);
    return *this;
}

ArchiveBase &TextArchive::serialize(uint16 &value)
{
    AChar buffer[7];
    serializeAsText(value, buffer, ARRAY_LENGTH(buffer), *this);
    return *this;
}

ArchiveBase &TextArchive::serialize(int8 &value)
{
    AChar buffer[5];
    serializeAsText(value, buffer, ARRAY_LENGTH(buffer), *this);
    return *this;
}

ArchiveBase &TextArchive::serialize(uint8 &value)
{
    AChar buffer[5];
    serializeAsText(value, buffer, ARRAY_LENGTH(buffer), *this);
    return *this;
}

ArchiveBase &TextArchive::serialize(TChar *value)
{
    // Up to 512 bytes in stack
    uint8 buffer[512];
    std::pmr::monotonic_buffer_resource memRes(buffer, ARRAY_LENGTH(buffer));

    // We always serialize as utf8
    if (isLoading())
    {
        SizeT len = lengthFromTextStream(*stream());

        std::pmr::string str{ &memRes };
        str.resize(len);
        // +1 To include null termination
        stream()->read(str.data(), len + 1);

        String outStr{ UTF8_TO_TCHAR(str.c_str()) };
        CBEMemory::memCopy(value, outStr.data(), outStr.length());
    }
    else
    {
        std::pmr::string writeStr{ TCHAR_TO_UTF8(value), &memRes };
        // Include null termination
        stream()->write(writeStr.data(), writeStr.length() + 1);
    }
    return *this;
}

ArchiveBase &TextArchive::serialize(String &value)
{
    // Up to 512 bytes in stack
    uint8 buffer[512];
    std::pmr::monotonic_buffer_resource memRes(buffer, ARRAY_LENGTH(buffer));

    // We always serialize as utf8
    if (isLoading())
    {
        SizeT len = lengthFromTextStream(*stream());

        std::pmr::string str{ &memRes };
        str.resize(len);
        // +1 To include null termination
        stream()->read(str.data(), len + 1);

        value = { UTF8_TO_TCHAR(str.c_str()) };
    }
    else
    {
        std::pmr::string writeStr{ TCHAR_TO_UTF8(value.getChar()), &memRes };
        // Include null termination
        stream()->write(writeStr.data(), writeStr.length() + 1);
    }
    return *this;
}

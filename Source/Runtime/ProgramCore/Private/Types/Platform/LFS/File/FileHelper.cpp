/*!
 * \file FileHelper.cpp
 *
 * \author Jeslas
 * \date February 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */
#include "Types/Platform/LFS/File/FileHelper.h"
#include "Types/Containers/ArrayView.h"
#include "Types/Platform/LFS/PlatformLFS.h"
#include "Types/Platform/PlatformFunctions.h"

// Encoding BOM(Byte Order Mark) https://docs.microsoft.com/en-us/globalization/encoding/byte-order-mark
bool FileHelper::isUtf8(const uint8 *byteStream, SizeT streamSize)
{
    // If BOM marked or is not other choices
    return isUtf8Bom(byteStream, streamSize)
           || !(
               isUtf16BEBom(byteStream, streamSize) || isUtf16LEBom(byteStream, streamSize) || isUtf32BEBom(byteStream, streamSize)
               || isUtf32LEBom(byteStream, streamSize)
           );
}

bool FileHelper::isUtf8Bom(const uint8 *byteStream, SizeT streamSize)
{
    return (streamSize > 3 && byteStream[0] == 0xEF && byteStream[1] == 0xBB && byteStream[2] == 0xBF);
}

bool FileHelper::isUtf16LEBom(const uint8 *byteStream, SizeT streamSize)
{
    // Has at least 2 header length and size if not odd
    return (streamSize > 2 && !(streamSize & 1) && byteStream[0] == 0xFF && byteStream[1] == 0xFE);
}

bool FileHelper::isUtf16BEBom(const uint8 *byteStream, SizeT streamSize)
{
    return (streamSize > 2 && !(streamSize & 1) && byteStream[0] == 0xFE && byteStream[1] == 0xFF);
}

bool FileHelper::isUtf32LEBom(const uint8 *byteStream, SizeT streamSize)
{
    return (
        streamSize > 4 && !(streamSize & 1) && byteStream[0] == 0xFF && byteStream[1] == 0xFE && byteStream[2] == 0x00 && byteStream[3] == 0x00
    );
}

bool FileHelper::isUtf32BEBom(const uint8 *byteStream, SizeT streamSize)
{
    return (
        streamSize > 4 && !(streamSize & 1) && byteStream[0] == 0x00 && byteStream[1] == 0x00 && byteStream[2] == 0xFE && byteStream[3] == 0xFF
    );
}

// TODO(Jeslas) : Replace this with compiler intrinsics if available
uint16 FileHelper::bytesSwap(uint16 value) { return (value << 8) | (value >> 8); }

uint32 FileHelper::bytesSwap(uint32 value)
{
    return (value >> 24) | ((value >> 8) & 0x0000FF00u) | ((value << 8) & 0x00FF0000u) | (value << 24);
}

uint64 FileHelper::bytesSwap(uint64 value)
{
    // Similar to merge sort, first swap smallest components then work your way up
    value = ((value >> 8) & 0x00FF00FF00FF00FFull) | ((value << 8) & 0xFF00FF00FF00FF00ull);
    value = ((value >> 16) & 0x0000FFFF0000FFFFull) | ((value << 16) & 0xFFFF0000FFFF0000ull);
    return (value >> 32) | (value << 32);
}

void FileHelper::bytesSwap(void *ptr, SizeT length)
{
    uint8 *bytePtr = (uint8 *)ptr;
    SizeT front = 0;
    SizeT back = length - 1;
    while (front < back)
    {
        std::swap(bytePtr[front++], bytePtr[back--]);
    }
}

bool FileHelper::readString(String &outStr, const String &fileName)
{
    std::vector<uint8> bytes;
    PlatformFile file(fileName);
    file.setSharingMode(EFileSharing::ReadOnly);
    file.setCreationAction(EFileFlags::OpenExisting);
    file.setFileFlags(EFileFlags::Read);
    if (file.openFile())
    {
        file.read(bytes);
        file.closeFile();
    }
    else
    {
        return false;
    }

    if (isUtf16LEBom(bytes.data(), bytes.size()))
    {
#if BIG_ENDIAN
        // If endian do not match swap it
        for (uint64 i = 0; i < bytes.size(); i += 2)
        {
            uint16 &charRef = *(uint16 *)(&bytes[i]);
            charRef = bytesSwap(charRef);
        }
#endif
        // Insert 2 null terminated characters
        bytes.insert(bytes.end(), 2, 0);
        outStr = UTF16_TO_TCHAR(&bytes[2]);
    }
    else if (isUtf16BEBom(bytes.data(), bytes.size()))
    {
#if LITTLE_ENDIAN
        // If endian do not match swap it
        for (uint64 i = 0; i < bytes.size(); i += 2)
        {
            uint16 &charRef = *(uint16 *)(&bytes[i]);
            charRef = bytesSwap(charRef);
        }
#endif
        // Insert 2 null terminated characters
        bytes.insert(bytes.end(), 2, 0);
        outStr = UTF16_TO_TCHAR(&bytes[2]);
    }
    else if (isUtf32LEBom(bytes.data(), bytes.size())) // Who wants to store in utf32? Right? rightttt?
    {
#if BIG_ENDIAN
        // If endian do not match swap it
        for (uint64 i = 0; i < bytes.size(); i += 4)
        {
            uint32 &charRef = *(uint32 *)(&bytes[i]);
            charRef = bytesSwap(charRef);
        }
#endif
        // Insert 4 null terminated characters
        bytes.insert(bytes.end(), 4, 0);
        outStr = UTF32_TO_TCHAR(&bytes[4]);
    }
    else if (isUtf32BEBom(bytes.data(), bytes.size())) // Again really?
    {
#if LITTLE_ENDIAN
        // If endian do not match swap it
        for (uint64 i = 0; i < bytes.size(); i += 4)
        {
            uint32 &charRef = *(uint32 *)(&bytes[i]);
            charRef = bytesSwap(charRef);
        }
#endif
        // Insert 4 null terminated characters
        bytes.insert(bytes.end(), 4, 0);
        outStr = UTF32_TO_TCHAR(&bytes[4]);
    }
    else
    {
        // Insert a null terminated characters
        bytes.emplace_back(0);
        outStr = UTF8_TO_TCHAR(&bytes[isUtf8Bom(bytes.data(), bytes.size()) ? 3 : 0]);
    }
    return true;
}

bool FileHelper::readUtf8String(std::string &outStr, const String &fileName)
{
    std::vector<uint8> bytes;
    PlatformFile file(fileName);
    file.setSharingMode(EFileSharing::ReadOnly);
    file.setCreationAction(EFileFlags::OpenExisting);
    file.setFileFlags(EFileFlags::Read);
    if (file.openFile())
    {
        file.read(bytes);
        file.closeFile();
    }
    else
    {
        return false;
    }

    if (isUtf16LEBom(bytes.data(), bytes.size()))
    {
#if BIG_ENDIAN
        // If endian do not match swap it
        for (uint64 i = 0; i < bytes.size(); i += 2)
        {
            uint16 &charRef = *(uint16 *)(&bytes[i]);
            charRef = bytesSwap(charRef);
        }
#endif
        outStr = UTF16_TO_UTF8(&bytes[2]);
    }
    else if (isUtf16BEBom(bytes.data(), bytes.size()))
    {
#if LITTLE_ENDIAN
        // If endian do not match swap it
        for (uint64 i = 0; i < bytes.size(); i += 2)
        {
            uint16 &charRef = *(uint16 *)(&bytes[i]);
            charRef = bytesSwap(charRef);
        }
#endif
        outStr = UTF16_TO_UTF8(&bytes[2]);
    }
    else if (isUtf32LEBom(bytes.data(), bytes.size())) // Who wants to store in utf32? Right? rightttt?
    {
#if BIG_ENDIAN
        // If endian do not match swap it
        for (uint64 i = 0; i < bytes.size(); i += 4)
        {
            uint32 &charRef = *(uint32 *)(&bytes[i]);
            charRef = bytesSwap(charRef);
        }
#endif
        outStr = UTF32_TO_UTF8(&bytes[4]);
    }
    else if (isUtf32BEBom(bytes.data(), bytes.size())) // Again really?
    {
#if LITTLE_ENDIAN
        // If endian do not match swap it
        for (uint64 i = 0; i < bytes.size(); i += 4)
        {
            uint32 &charRef = *(uint32 *)(&bytes[i]);
            charRef = bytesSwap(charRef);
        }
#endif
        outStr = UTF32_TO_UTF8(&bytes[4]);
    }
    else
    {
        outStr = reinterpret_cast<const AChar *>(&bytes[isUtf8Bom(bytes.data(), bytes.size()) ? 3 : 0]);
    }
    return true;
}

bool FileHelper::readBytes(std::vector<uint8> &outBytes, const String &fileName)
{
    PlatformFile file(fileName);
    file.setSharingMode(EFileSharing::ReadOnly);
    file.setCreationAction(EFileFlags::OpenExisting);
    file.setFileFlags(EFileFlags::Read);
    if (file.openFile())
    {
        file.read(outBytes);
        file.closeFile();
        return true;
    }
    return false;
}

bool FileHelper::writeString(const String &content, const String &fileName)
{
    std::string utf8Str{ TCHAR_TO_UTF8(content.getChar()) };

    PlatformFile file(fileName);
    file.setSharingMode(EFileSharing::ReadOnly);
    file.setCreationAction(EFileFlags::CreateAlways);
    file.setFileFlags(EFileFlags::Write);
    if (file.openOrCreate())
    {
        file.write(ArrayView<const uint8>(reinterpret_cast<uint8 *>(utf8Str.data()), utf8Str.size()));
        file.closeFile();
        return true;
    }
    return false;
}

bool FileHelper::writeBytes(const std::vector<uint8> &bytes, const String &fileName)
{
    PlatformFile file(fileName);
    file.setSharingMode(EFileSharing::ReadOnly);
    file.setCreationAction(EFileFlags::CreateAlways);
    file.setFileFlags(EFileFlags::Write);
    if (file.openOrCreate())
    {
        file.write(ArrayView<const uint8>(bytes.data(), bytes.size()));
        file.closeFile();
        return true;
    }
    return false;
}

bool FileHelper::touchFile(const String &fileName)
{
    PlatformFile file(fileName);
    file.setSharingMode(EFileSharing::ReadOnly);
    file.setCreationAction(EFileFlags::OpenAlways);
    file.setFileFlags(EFileFlags::Write);

    bool bSuccess = false;
    if (file.openOrCreate())
    {
        bSuccess = file.setLastWriteTimeStamp(Time::clockTimeNow());
        file.closeFile();
    }
    return bSuccess;
}

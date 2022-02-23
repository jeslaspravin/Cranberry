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
#include "Types/Platform/LFS/PlatformLFS.h"
#include "Types/Platform/PlatformFunctions.h"
#include "Types/Containers/ArrayView.h"

// Encoding BOM(Byte Order Mark) https://docs.microsoft.com/en-us/globalization/encoding/byte-order-mark
bool FileHelper::isUtf8(const std::vector<uint8>& byteStream)
{
    // If BOM marked or is not other choices
    return isUtf8Bom(byteStream) || !(isUtf16BEBom(byteStream) || isUtf16LEBom(byteStream) 
        || isUtf32BEBom(byteStream) || isUtf32LEBom(byteStream));
}

bool FileHelper::isUtf8Bom(const std::vector<uint8>& byteStream)
{
    return (byteStream.size() > 3 && byteStream[0] == 0xEF && byteStream[1] == 0xBB && byteStream[2] == 0xBF);
}

bool FileHelper::isUtf16LEBom(const std::vector<uint8>& byteStream)
{
    // Has at least 2 header length and size if not odd
    return (byteStream.size() > 2 && !(byteStream.size() & 1)
        && byteStream[0] == 0xFF && byteStream[1] == 0xFE);
}

bool FileHelper::isUtf16BEBom(const std::vector<uint8>& byteStream)
{
    return (byteStream.size() > 2 && !(byteStream.size() & 1)
        && byteStream[0] == 0xFE && byteStream[1] == 0xFF);
}

bool FileHelper::isUtf32LEBom(const std::vector<uint8>& byteStream)
{
    return (byteStream.size() > 4 && !(byteStream.size() & 1)
        && byteStream[0] == 0xFF && byteStream[1] == 0xFE
        && byteStream[2] == 0x00 && byteStream[3] == 0x00);
}

bool FileHelper::isUtf32BEBom(const std::vector<uint8>& byteStream)
{
    return (byteStream.size() > 4 && !(byteStream.size() & 1)
        && byteStream[0] == 0x00 && byteStream[1] == 0x00
        && byteStream[2] == 0xFE && byteStream[3] == 0xFF);
}

bool FileHelper::readString(String& outStr, const String& fileName)
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

    if (isUtf16LEBom(bytes))
    {
        // If endian do not match swap it
        if (GPlatformConfigs::PLATFORM_ENDIAN.isBigEndian())
        {
            for (uint64 i = 0; i < bytes.size(); i += 2)
            {
                std::swap(bytes[i], bytes[i + 1]);
            }
        }
        // Insert 2 null terminated characters
        bytes.insert(bytes.end(), 2, 0);
        outStr = UTF16_TO_TCHAR(&bytes[2]);
    }
    else if (isUtf16BEBom(bytes))
    {
        // If endian do not match swap it
        if (GPlatformConfigs::PLATFORM_ENDIAN.isLittleEndian())
        {
            for (uint64 i = 0; i < bytes.size(); i += 2)
            {
                std::swap(bytes[i], bytes[i + 1]);
            }
        }
        // Insert 2 null terminated characters
        bytes.insert(bytes.end(), 2, 0);
        outStr = UTF16_TO_TCHAR(&bytes[2]);
    } 
    else if (isUtf32LEBom(bytes)) // Who wants to store in utf32? Right? rightttt?
    {
        // If endian do not match swap it
        if (GPlatformConfigs::PLATFORM_ENDIAN.isBigEndian())
        {
            for (uint64 i = 0; i < bytes.size(); i += 4)
            {
                std::swap(bytes[i], bytes[i + 3]);
                std::swap(bytes[i + 1], bytes[i + 2]);
            }
        }
        // Insert 4 null terminated characters
        bytes.insert(bytes.end(), 4, 0);
        outStr = UTF32_TO_TCHAR(&bytes[4]);
    }
    else if (isUtf32LEBom(bytes)) // Again really?
    {
        // If endian do not match swap it
        if (GPlatformConfigs::PLATFORM_ENDIAN.isLittleEndian())
        {
            for (uint64 i = 0; i < bytes.size(); i += 4)
            {
                std::swap(bytes[i], bytes[i + 3]);
                std::swap(bytes[i + 1], bytes[i + 2]);
            }
        }
        // Insert 4 null terminated characters
        bytes.insert(bytes.end(), 4, 0);
        outStr = UTF32_TO_TCHAR(&bytes[4]);
    }
    else
    {
        // Insert a null terminated characters
        bytes.emplace_back(0);
        outStr = UTF8_TO_TCHAR(&bytes[isUtf8Bom(bytes) ? 3 : 0]);
    }
    return true;
}

bool FileHelper::readUtf8String(std::string& outStr, const String& fileName)
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

    if (isUtf16LEBom(bytes))
    {
        // If endian do not match swap it
        if (GPlatformConfigs::PLATFORM_ENDIAN.isBigEndian())
        {
            for (uint64 i = 0; i < bytes.size(); i += 2)
            {
                std::swap(bytes[i], bytes[i + 1]);
            }
        }
        outStr = UTF16_TO_UTF8(&bytes[2]);
    }
    else if (isUtf16BEBom(bytes))
    {
        // If endian do not match swap it
        if (GPlatformConfigs::PLATFORM_ENDIAN.isLittleEndian())
        {
            for (uint64 i = 0; i < bytes.size(); i += 2)
            {
                std::swap(bytes[i], bytes[i + 1]);
            }
        }
        outStr = UTF16_TO_UTF8(&bytes[2]);
    }
    else if (isUtf32LEBom(bytes)) // Who wants to store in utf32? Right? rightttt?
    {
        // If endian do not match swap it
        if (GPlatformConfigs::PLATFORM_ENDIAN.isBigEndian())
        {
            for (uint64 i = 0; i < bytes.size(); i += 4)
            {
                std::swap(bytes[i], bytes[i + 3]);
                std::swap(bytes[i + 1], bytes[i + 2]);
            }
        }
        outStr = UTF32_TO_UTF8(&bytes[4]);
    }
    else if (isUtf32LEBom(bytes)) // Again really?
    {
        // If endian do not match swap it
        if (GPlatformConfigs::PLATFORM_ENDIAN.isLittleEndian())
        {
            for (uint64 i = 0; i < bytes.size(); i += 4)
            {
                std::swap(bytes[i], bytes[i + 3]);
                std::swap(bytes[i + 1], bytes[i + 2]);
            }
        }
        outStr = UTF32_TO_UTF8(&bytes[4]);
    }
    else
    {
        outStr = reinterpret_cast<const AChar*>(&bytes[isUtf8Bom(bytes) ? 3 : 0]);
    }
    return true;
}

bool FileHelper::readBytes(std::vector<uint8>& outBytes, const String& fileName)
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

bool FileHelper::writeString(const String& content, const String& fileName)
{
    std::string utf8Str{ TCHAR_TO_UTF8(content.getChar()) };

    PlatformFile file(fileName);
    file.setSharingMode(EFileSharing::ReadOnly);
    file.setCreationAction(EFileFlags::CreateAlways);
    file.setFileFlags(EFileFlags::Write);
    if (file.openOrCreate())
    {
        file.write(ArrayView<uint8>(reinterpret_cast<uint8*>(utf8Str.data()), utf8Str.size()));
        file.closeFile();
        return true;
    }
    return false;
}

bool FileHelper::writeBytes(std::vector<uint8>& bytes, const String& fileName)
{
    PlatformFile file(fileName);
    file.setSharingMode(EFileSharing::ReadOnly);
    file.setCreationAction(EFileFlags::CreateAlways);
    file.setFileFlags(EFileFlags::Write);
    if (file.openOrCreate())
    {
        file.write(ArrayView<uint8>(bytes.data(), bytes.size()));
        file.closeFile();
        return true;
    }
    return false;
}

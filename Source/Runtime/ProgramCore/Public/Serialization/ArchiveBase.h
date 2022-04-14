/*!
 * \file ArchiveBase.h
 *
 * \author Jeslas
 * \date March 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "ProgramCoreExports.h"
#include "Types/CoreDefines.h"
#include "Types/CoreTypes.h"

#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

class String;

// Source stream of data for archive to read or write
// Each read or write pushes the stream forward or backwards
class PROGRAMCORE_EXPORT ArchiveStream
{
public:
    // reads given length of data from cursor to the passed in pointer, Pointer must be pointing to data
    // with at least len size Moves the stream cursor to start of next data stream
    virtual void read(void *toPtr, SizeT len) = 0;
    // writes given length of data from cursor from the passed in pointer to data stream, Pointer must be
    // pointing to data with at least len size Moves the stream cursor to start of next write data
    // stream. Allocates or extends any necessary extra stream data required for this write.
    virtual void write(const void *ptr, SizeT len) = 0;

    // Moves the stream cursor forward by count bytes
    // Allocates or extends any necessary extra stream data required for this write.
    virtual void moveForward(SizeT count) = 0;
    // Moves the stream cursor backward by count bytes
    virtual void moveBackward(SizeT count) = 0;

    // Preallocates addition count bytes in buffered streams, Does not modify the stream cursor
    virtual bool allocate(SizeT count) = 0;

    // Reads byte at idx forward from current byte, Does not modify the stream cursor
    virtual uint8 readForwardAt(SizeT idx) const = 0;
    // Reads byte at idx backward from current byte, Does not modify the stream cursor
    virtual uint8 readBackwardAt(SizeT idx) const = 0;

    virtual bool isAvailable() const = 0;

    virtual ~ArchiveStream() = default;
};

#define SERIALIZE_VIRTUAL(TypeName) virtual ArchiveBase &serialize(TypeName &value) = 0;

class PROGRAMCORE_EXPORT ArchiveBase
{
private:
    // If byte swapping is necessary for this archive?
    bool bShouldSwapBytes = false;
    bool bIsLoading = false;

    // Borrowed stream(Ownership must below to creator)
    ArchiveStream *archiveStream = nullptr;

public:
    FORCE_INLINE bool ifSwapBytes() const { return bShouldSwapBytes; }
    FORCE_INLINE void setSwapBytes(bool bSwapBytes) { bShouldSwapBytes = bSwapBytes; }
    FORCE_INLINE bool isLoading() const { return bIsLoading; }
    FORCE_INLINE void setLoading(bool bLoad) { bIsLoading = bLoad; }
    FORCE_INLINE ArchiveStream *stream() const { return archiveStream; }
    FORCE_INLINE void setStream(ArchiveStream *inStream) { archiveStream = inStream; }

    FOR_EACH_CORE_TYPES(SERIALIZE_VIRTUAL)

    virtual ArchiveBase &serialize(String &) = 0;
    virtual ArchiveBase &serialize(TChar *) = 0;
};

#undef SERIALIZE_VIRTUAL

template <typename Type>
concept IsArchiveType = std::is_base_of_v<ArchiveBase, Type>;

template <IsArchiveType ArchiveType, typename ValueType>
ArchiveType &operator<<(ArchiveType &archive, ValueType &value)
{
    return static_cast<ArchiveType &>(archive.serialize(value));
}

template <IsArchiveType ArchiveType, typename ValueType>
ArchiveType &operator<<(ArchiveType &archive, ValueType *value)
{
    static_assert(false,
        "Pointer type serialization is not supported! Specialize and provide your own serialization");
    return archive;
}

template <IsArchiveType ArchiveType, typename KeyType, typename ValueType>
ArchiveType &operator<<(ArchiveType &archive, std::pair<KeyType, ValueType> &value)
{
    return archive << value.first << value.second;
}

template <IsArchiveType ArchiveType, typename ValueType, typename AllocType>
ArchiveType &operator<<(ArchiveType &archive, std::vector<ValueType, AllocType> &value)
{
    SizeT len = value.size();
    archive << len;
    if (archive.isLoading())
    {
        value.resize(len);
    }

    for (SizeT i = 0; i < len; ++i)
    {
        archive << value[i];
    }
    return archive;
}

template <IsArchiveType ArchiveType, typename KeyType, typename... Types>
ArchiveType &operator<<(ArchiveType &archive, std::set<KeyType, Types...> &value)
{
    SizeT len = value.size();
    archive << len;
    if (archive.isLoading())
    {
        SizeT i = 0;
        while (i < len)
        {
            KeyType key;
            archive << key;
            value.insert(key);
            ++i;
        }
    }
    else
    {
        for (const auto &val : value)
        {
            archive << val;
        }
    }
    return archive;
}
template <IsArchiveType ArchiveType, typename KeyType, typename... Types>
ArchiveType &operator<<(ArchiveType &archive, std::unordered_set<KeyType, Types...> &value)
{
    SizeT len = value.size();
    archive << len;
    if (archive.isLoading())
    {
        value.reserve(len);
        SizeT i = 0;
        while (i < len)
        {
            KeyType key;
            archive << key;
            value.insert(key);
            ++i;
        }
    }
    else
    {
        for (const auto &val : value)
        {
            archive << val;
        }
    }
    return archive;
}

template <IsArchiveType ArchiveType, typename KeyType, typename ValueType, typename... Types>
ArchiveType &operator<<(ArchiveType &archive, std::map<KeyType, ValueType, Types...> &value)
{
    SizeT len = value.size();
    archive << len;
    if (archive.isLoading())
    {
        SizeT i = 0;
        while (i < len)
        {
            KeyType key;
            ValueType val;
            archive << key << val;
            value[key] = val;
            ++i;
        }
    }
    else
    {
        for (const auto &pair : value)
        {
            archive << pair.first << pair.second;
        }
    }
    return archive;
}
template <IsArchiveType ArchiveType, typename KeyType, typename ValueType, typename... Types>
ArchiveType &operator<<(ArchiveType &archive, std::unordered_map<KeyType, ValueType, Types...> &value)
{
    SizeT len = value.size();
    archive << len;
    if (archive.isLoading())
    {
        value.reserve(len);
        SizeT i = 0;
        while (i < len)
        {
            KeyType key;
            ValueType val;
            archive << key << val;
            value[key] = val;
            ++i;
        }
    }
    else
    {
        for (const auto &pair : value)
        {
            archive << pair.first << pair.second;
        }
    }
    return archive;
}
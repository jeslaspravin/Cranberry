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
#include "Serialization/ArchiveTypes.h"
#include "Math/Math.h"
#include "Types/CoreDefines.h"
#include "Types/CoreTypes.h"
#include "Types/Templates/TemplateTypes.h"
#include "Logger/Logger.h"

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
    ArchiveStream() = default;
    MAKE_TYPE_NONCOPY_NONMOVE(ArchiveStream)

    virtual ~ArchiveStream() = default;

public:
    // reads given length of data from cursor to the passed in pointer, Pointer must be pointing to data
    // with at least len size Moves the stream cursor to start of next data stream
    virtual void read(void *toPtr, SizeT byteLen) = 0;
    // writes given length of data from cursor from the passed in pointer to data stream, Pointer must be
    // pointing to data with at least len size Moves the stream cursor to start of next write data
    // stream. Allocates or extends any necessary extra stream data required for this write.
    virtual void write(const void *ptr, SizeT byteLen) = 0;

    // Moves the stream cursor forward by count bytes
    // Allocates or extends any necessary extra stream data required for this write.
    virtual void moveForward(SizeT byteCount) = 0;
    // Moves the stream cursor backward by count bytes
    virtual void moveBackward(SizeT byteCount) = 0;

    // Preallocates addition count bytes in buffered streams, Does not modify the stream cursor
    virtual bool allocate(SizeT byteCount) = 0;

    // Reads byte at idx forward from current byte, Does not modify the stream cursor
    virtual uint8 readForwardAt(SizeT idx) const = 0;
    // Reads byte at idx backward from current byte, Does not modify the stream cursor
    virtual uint8 readBackwardAt(SizeT idx) const = 0;

    virtual uint64 cursorPos() const = 0;

    virtual bool isAvailable() const = 0;
    virtual bool hasMoreData(SizeT requiredByteCount) const = 0;
};

class PROGRAMCORE_EXPORT ArchiveSizeCounterStream final : public ArchiveStream
{
private:
    uint64 cursor;

public:
    ArchiveSizeCounterStream()
        : cursor(0)
    {}

    /* ArchiveStream overrides */
    void read(void *toPtr, SizeT byteLen) override;
    void write(const void *ptr, SizeT byteLen) override { cursor += byteLen; }
    void moveForward(SizeT byteCount) override { cursor += byteCount; }
    void moveBackward(SizeT byteCount) override { cursor = Math::max(cursor - byteCount, 0); }
    bool allocate(SizeT byteCount) override { return false; }
    uint8 readForwardAt(SizeT idx) const override;
    uint8 readBackwardAt(SizeT idx) const override;
    FORCE_INLINE uint64 cursorPos() const override { return cursor; }
    FORCE_INLINE bool isAvailable() const override { return true; }
    bool hasMoreData(SizeT requiredByteCount) const override { return true; }
    /* overrides ends */
};

#define SERIALIZE_VIRTUAL(TypeName) virtual ArchiveBase &serialize(TypeName &value) = 0;

class PROGRAMCORE_EXPORT ArchiveBase
{
private:
    constexpr static const uint32 ARCHIVE_VERSION = 0;
    /*
     * Lowest version supported
     */
    constexpr static const uint32 CUTOFF_VERSION = 0;

    // Custom versions that will be serialized in/from this archive
    std::map<uint32, uint32> customVersions;

    // If byte swapping is necessary for this archive?
    bool bShouldSwapBytes = false;
    bool bIsLoading = false;

    // Borrowed stream(Ownership must belong to creator)
    ArchiveStream *archiveStream = nullptr;

public:
    /**
     * All getters are virtual to allow overriding the behavior however setters are not as that needs to taken care by the user directly and set
     * values to appropriate archives, Example check ObjectArchive and PackageSaver. 
     * It manually takes care of setting custom versions in each archive based on loading or saving
     */
    virtual bool ifSwapBytes() const { return bShouldSwapBytes; }
    virtual bool isLoading() const { return bIsLoading; }
    virtual ArchiveStream *stream() const { return archiveStream; }
    virtual uint32 getCustomVersion(uint32 customId) const
    {
        auto itr = customVersions.find(customId);
        if (itr != customVersions.cend())
        {
            return itr->second;
        }
        return 0;
    }
    virtual const std::map<uint32, uint32> &getCustomVersions() const { return customVersions; }

    FORCE_INLINE void setSwapBytes(bool bSwapBytes) { bShouldSwapBytes = bSwapBytes; }
    FORCE_INLINE void setLoading(bool bLoad) { bIsLoading = bLoad; }
    FORCE_INLINE void setStream(ArchiveStream *inStream)
    {
        if (archiveStream == inStream)
        {
            return;
        }

        archiveStream = inStream;
        if (archiveStream)
        {
            uint64 currCursor = archiveStream->cursorPos();
            if (currCursor != 0)
            {
                archiveStream->moveBackward(currCursor);
            }
            serializeArchiveMeta();
            if (currCursor != 0)
            {
                archiveStream->moveForward(currCursor);
            }
        }
    }
    FORCE_INLINE void setCustomVersion(uint32 customId, uint32 version) { customVersions[customId] = version; }
    FORCE_INLINE void clearCustomVersions() { customVersions.clear(); }

    FOR_EACH_CORE_TYPES(SERIALIZE_VIRTUAL)

    virtual ArchiveBase &serialize(String &) = 0;
    virtual ArchiveBase &serialize(TChar *) = 0;

private:
    void serializeArchiveMeta();
};

#undef SERIALIZE_VIRTUAL

template <ArchiveTypeName ArchiveType, typename ValueType>
ArchiveType &operator<<(ArchiveType &archive, ValueType &value)
{
    return static_cast<ArchiveType &>(archive.serialize(value));
}

template <ArchiveTypeName ArchiveType, typename KeyType, typename ValueType>
ArchiveType &operator<<(ArchiveType &archive, std::pair<KeyType, ValueType> &value)
{
    return archive << value.first << value.second;
}

template <ArchiveTypeName ArchiveType, typename ValueType, typename AllocType>
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

template <ArchiveTypeName ArchiveType, typename KeyType, typename... Types>
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
        for (const KeyType &val : value)
        {
            archive << *const_cast<std::remove_const_t<KeyType> *>(&val);
        }
    }
    return archive;
}
template <ArchiveTypeName ArchiveType, typename KeyType, typename... Types>
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
        for (const KeyType &val : value)
        {
            archive << *const_cast<std::remove_const_t<KeyType> *>(&val);
        }
    }
    return archive;
}

template <ArchiveTypeName ArchiveType, typename KeyType, typename ValueType, typename... Types>
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
        for (std::pair<const KeyType, ValueType> &pair : value)
        {
            archive << *const_cast<std::remove_const_t<KeyType> *>(&pair.first) << pair.second;
        }
    }
    return archive;
}
template <ArchiveTypeName ArchiveType, typename KeyType, typename ValueType, typename... Types>
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
        for (std::pair<const KeyType, ValueType> &pair : value)
        {
            archive << *const_cast<std::remove_const_t<KeyType> *>(&pair.first) << pair.second;
        }
    }
    return archive;
}
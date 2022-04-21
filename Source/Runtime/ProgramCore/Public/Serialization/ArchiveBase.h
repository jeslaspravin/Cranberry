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
#include "Types/Templates/TemplateTypes.h"

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

    virtual uint64 cursorPos() const = 0;

    virtual bool isAvailable() const = 0;

    virtual ~ArchiveStream() = default;
};

class PROGRAMCORE_EXPORT ArchiveSizeCounterStream final : public ArchiveStream
{
private:
    uint64 cursor;

public:
    /* ArchiveStream overrides */
    void read(void *toPtr, SizeT len) override;
    FORCE_INLINE void write(const void *ptr, SizeT len) override { cursor += len; }
    FORCE_INLINE void moveForward(SizeT count) override { cursor += count; }
    FORCE_INLINE void moveBackward(SizeT count) override { cursor += count; }
    FORCE_INLINE bool allocate(SizeT count) override { return false; }
    uint8 readForwardAt(SizeT idx) const override;
    uint8 readBackwardAt(SizeT idx) const override;
    FORCE_INLINE uint64 cursorPos() const override { return cursor; }
    FORCE_INLINE bool isAvailable() const override { return true; }
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

private:
    void serializeArchiveMeta();

public:
    FORCE_INLINE bool ifSwapBytes() const { return bShouldSwapBytes; }
    FORCE_INLINE void setSwapBytes(bool bSwapBytes) { bShouldSwapBytes = bSwapBytes; }
    FORCE_INLINE bool isLoading() const { return bIsLoading; }
    FORCE_INLINE void setLoading(bool bLoad) { bIsLoading = bLoad; }
    FORCE_INLINE ArchiveStream *stream() const { return archiveStream; }
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
    FORCE_INLINE uint32 getCustomVersion(uint32 customId) const
    {
        auto itr = customVersions.find(customId);
        if (itr != customVersions.cend())
        {
            return itr->second;
        }
        return 0;
    }
    FORCE_INLINE const std::map<uint32, uint32> &getCustomVersions() const { return customVersions; }
    FORCE_INLINE void clearCustomVersions() { customVersions.clear(); }

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
    // This is added to support writing keys of maps and sets
    // #TODO(Jeslas) : Should I handle const here or just handle directly in special cases line set or map elements?
    if constexpr (std::is_const_v<ValueType>)
    {
        return static_cast<ArchiveType &>(archive.serialize(*const_cast<std::remove_const_t<ValueType> *>(&value)));
    }
    else
    {
        return static_cast<ArchiveType &>(archive.serialize(value));
    }
}

template <IsArchiveType ArchiveType, typename ValueType>
ArchiveType &operator<<(ArchiveType &archive, ValueType *value)
{
    static_assert(
        DependentFalseTypeValue<ValueType>, "Pointer type serialization is not supported! Specialize and provide your own serialization"
    );
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
        for (KeyType val : value)
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
        for (KeyType val : value)
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
        for (const std::pair<KeyType, ValueType> &pair : value)
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
        for (const std::pair<KeyType, ValueType> &pair : value)
        {
            archive << pair.first << pair.second;
        }
    }
    return archive;
}
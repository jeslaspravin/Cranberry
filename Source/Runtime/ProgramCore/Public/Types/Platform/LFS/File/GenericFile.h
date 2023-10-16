/*!
 * \file GenericFile.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once
#include "ProgramCoreExports.h"
#include "String/String.h"
#include "Types/Containers/ArrayView.h"
#include "Types/Platform/PlatformTypes.h"
#include "Types/Time.h"

#include <memory>

namespace EFileFlags
{
enum EFileFlags : uint8
{
    None = 0x00,
    Read = 0x01,
    Write = 0x02,
    Execute = 0x04,
    // Create or Open only flags
    CreateNew = 0x08,    // Creates only if not existing
    CreateAlways = 0x10, // Creates no matter if exists or not
    OpenExisting = 0x20, // Opens only if existing
    OpenAlways = 0x40,   // Creates or opens
    ClearExisting = 0x80 // Opens and clears only if existing
};
} // namespace EFileFlags

namespace FileFlags
{
const uint8 ACCESS_FLAGS = (EFileFlags::Read | EFileFlags::Write | EFileFlags::Execute);
const uint8 OPEN_ACTION_FLAGS = 0xFF & ~ACCESS_FLAGS;
} // namespace FileFlags

namespace EFileSharing
{
enum EFileSharing : uint8
{
    NoSharing = 0x00,
    DeleteOnly = 0x01,
    ReadOnly = 0x02,
    WriteOnly = 0x04
};
} // namespace EFileSharing

namespace EFileAdditionalFlags
{
enum EFileAdditionalFlags : uint32
{
    // File property flags
    Normal = 0x00000000,    // Default, No special attributes
    Archive = 0x00000001,   // For file that are backup or to be removed
    Encrypted = 0x00000002, // Encrypted file only during creation
    Hidden = 0x00000004,    // Hidden not to be used with directory
    ReadOnly = 0x00000008,  // App can only read from file
    System = 0x00000010,    // System files
    Temporary = 0x00000020, // Temporary files
    // Data property flags
    Backup = 0x00000040,           // Backup file,Overrides security check
    TemporaryDelete = 0x00000080,  // Temporary files, Gets deleted after usage of handle.
    NoBuffering = 0x00000100,      // No system level data caching
    OpenRemoteOnly = 0x00000200,   // Always access remote storage and not stored in local storage
    AsyncOverlapped = 0x00000400,  // for async operation try to use overlapped if possible.
    Posix = 0x00000800,            // POSIX semantics
    RandomAccess = 0x00001000,     // Random access to file data possible
    SequentialAccess = 0x00002000, // Random access to file data not possible
    WriteDirectDisk = 0x00004000,  // Writes directly to the disk.
};
} // namespace EFileAdditionalFlags

class PROGRAMCORE_EXPORT GenericFile
{

protected:
    PlatformHandle fileHandle;
    String directoryPath;
    // Empty in case of directory
    String fileName;
    String fullPath;

    // EFileFlags
    uint8 fileFlags;
    // EFileSharing
    uint8 sharingMode;
    // EFileAdditionalFlags
    uint32 attributes;
    // Platform specific flags do not use if cross platform and not sure if needed
    uint64 advancedFlags;

protected:
    FORCE_INLINE PlatformHandle getFileHandle() const { return fileHandle; }

    virtual PlatformHandle openOrCreateImpl() = 0;
    virtual PlatformHandle openImpl() const = 0;
    // Must flush if necessary
    virtual bool closeImpl() const = 0;
    virtual bool dirDelete() const = 0;
    virtual bool dirClearAndDelete() const = 0;

    void setPath(const String &fPath);

public:
    GenericFile();
    GenericFile(const String &path);

    virtual ~GenericFile() = default;

    // Opens only if previous file is properly closed
    // The goal of this function is to create or open irrespective of setting flags and still maintain
    // the setting to user set if possible
    bool openOrCreate();
    bool openFile();
    // Closes the file if it exists
    bool closeFile();

    virtual void flush() const = 0;
    NODISCARD bool exists() const;

    NODISCARD bool isDirectory() const;
    NODISCARD bool isFile() const;

    // Name of file if any(File name is one with .ext)
    const String &getFileName() const;
    // Path of parent directory ignores file name if file or last directory name in path if directory
    // path
    const String &getHostDirectory() const;
    // Returns last directory name of full directory path
    NODISCARD String getDirectoryName() const;
    const String &getFullPath() const;

    // Direct OS specific flags maps directly to OS bits
    void setAdvancedFlags(uint64 flags) { advancedFlags = flags; }
    // Sharing mode for other handles
    void setSharingMode(uint8 sharingFlags) { sharingMode = sharingFlags; }
    // How to open the file (Read or write or both)
    void setFileFlags(uint8 flags);
    // File specific attribute(Encodings and such)
    void setAttributes(uint32 attribs) { attributes = attribs; }
    // File on open or creation actions
    void setCreationAction(uint8 creationAction);

    void addAdvancedFlags(uint64 flags);
    void removeAdvancedFlags(uint64 flags);

    void addSharingFlags(uint8 sharingFlags);
    void removeSharingFlags(uint8 sharingFlags);

    void addFileFlags(uint8 flags);
    void removeFileFlags(uint8 flags);

    void addAttributes(uint32 attribs);
    void removeAttributes(uint32 attribs);

    // Operations after file handle is created

    virtual TickRep lastWriteTimeStamp() const = 0;
    virtual bool setLastWriteTimeStamp(TickRep timeTick) const = 0;
    virtual TickRep createTimeStamp() const = 0;
    virtual uint64 fileSize() const = 0;
    virtual uint64 filePointer() const = 0;
    virtual void seekEnd() const = 0;
    virtual void seekBegin() const = 0;
    virtual void seek(int64 pointer) const = 0;
    virtual void offsetCursor(int64 offset) const = 0;

    virtual bool setFileSize(int64 newSize) const = 0;
    virtual void read(std::vector<uint8> &readTo, uint32 bytesToRead = (~0u)) const = 0;
    virtual void read(uint8 *readTo, uint32 bytesToRead) const = 0;
    virtual void write(ArrayView<uint8> writeBytes) const = 0;

    virtual bool deleteFile() = 0;
    virtual bool renameFile(String newName) = 0;

    // Works only if directory
    template <bool ClearFiles>
    bool deleteDirectory() const;

    template <>
    bool deleteDirectory<true>() const
    {
        return dirClearAndDelete();
    }

    template <>
    bool deleteDirectory<false>() const
    {
        return dirDelete();
    }

    virtual bool createDirectory() const = 0;
};

namespace std
{
template <>
struct PROGRAMCORE_EXPORT default_delete<GenericFile>
{
    void operator() (GenericFile *_Ptr) const noexcept
    {
        _Ptr->closeFile();
        delete _Ptr;
    }
};
} // namespace std
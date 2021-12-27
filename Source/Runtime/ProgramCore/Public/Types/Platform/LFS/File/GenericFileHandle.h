#pragma once
#include "Types/CoreTypes.h"
#include "ProgramCoreExports.h"

namespace EFileFlags
{
    enum EFileFlags : uint8
    {
        None = 0x00,
        Read = 0x01,
        Write = 0x02,
        Execute = 0x04,
        // Create or Open only flags
        CreateNew = 0x08,// Creates only if not existing
        CreateAlways = 0x10,// Creates no matter if exists or not
        OpenExisting = 0x20,// Opens only if existing
        OpenAlways = 0x40,// Creates or opens
        ClearExisting = 0x80 // Opens and clears only if existing
    };
}

namespace FileFlags 
{
    const uint8 ACCESS_FLAGS = (EFileFlags::Read | EFileFlags::Write | EFileFlags::Execute);
    const uint8 OPEN_ACTION_FLAGS = 0xFF & ~ACCESS_FLAGS;
}

namespace EFileSharing
{
    enum EFileSharing : uint8 
    {
        NoSharing = 0x00,
        DeleteOnly = 0x01,
        ReadOnly = 0x02,
        WriteOnly = 0x04
    };
}

namespace EFileAdditionalFlags
{
    enum EFileAdditionalFlags : uint32 
    {
        // File property flags
        Normal = 0x00000000,// Default, No special attributes
        Archive = 0x00000001,// For file that are backup or to be removed
        Encrypted = 0x00000002,// Encrypted file only during creation
        Hidden = 0x00000004,// Hidden not to be used with directory
        ReadOnly = 0x00000008,// App can only read from file
        System = 0x00000010,// System files
        Temporary = 0x00000020,// Temporary files
        // Data property flags
        Backup = 0x00000040,// Backup file,Overrides security check
        TemporaryDelete = 0x00000080,// Temporary files, Gets deleted after usage of handle.
        NoBuffering = 0x00000100,// No system level data caching
        OpenRemoteOnly = 0x00000200,// Always access remote storage and not stored in local storage
        AsyncOverlapped = 0x00000400,// for async operation try to use overlapped if possible.
        Posix = 0x00000800,// POSIX semantics
        RandomAccess = 0x00001000,// Random access to file data possible
        SequentialAccess = 0x00002000,// Random access to file data not possible
        WriteDirectDisk = 0x00004000,// Writes directly to the disk.
    };
}

class GenericFile;

class PROGRAMCORE_EXPORT GenericFileHandle 
{

    friend GenericFile;

protected:
    virtual void* getFileHandle() = 0;

    uint8 fileFlags;
    uint8 fileSharing;
    uint32 fileExtraFlags;
    uint64 rawFileFlags;

public:

    GenericFileHandle(uint8 flags, uint8 sharing = (uint8)EFileSharing::ReadOnly,
        uint32 extraFlags = 0, uint64 advancedFlags = 0) :
        fileFlags(flags),
        fileSharing(sharing),
        fileExtraFlags(extraFlags),
        rawFileFlags(advancedFlags)
    {}

    virtual ~GenericFileHandle() = default;

};
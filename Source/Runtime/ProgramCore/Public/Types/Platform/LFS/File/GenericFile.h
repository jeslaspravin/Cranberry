#pragma once
#include "String/String.h"
#include "Types/Containers/ArrayView.h"
#include "ProgramCoreExports.h"

#include <memory>

class GenericFileHandle;

class PROGRAMCORE_EXPORT GenericFile 
{
    
protected:

    GenericFileHandle* fileHandle;
    String directoryPath;
    // Empty in case of directory
    String fileName;
    String fullPath;

    uint8 fileFlags;
    uint8 sharingMode;
    uint32 attributes;
    uint64 advancedFlags;

protected:

    void* getFileHandleRaw() const;
    GenericFileHandle* getFileHandle() const { return fileHandle; }

    virtual GenericFileHandle* openOrCreateImpl()=0;
    virtual GenericFileHandle* openImpl()  const = 0;
    // Must flush if necessary
    virtual bool closeImpl() const = 0;
    virtual bool dirDelete() const = 0;
    virtual bool dirClearAndDelete() const = 0;

    void setPath(const String& fPath);

public:
    GenericFile();
    GenericFile(const String& path);

    virtual ~GenericFile() = default;

    // Opens only if previous file is properly closed
    bool openOrCreate();
    bool openFile();
    // Closes the file if it exists 
    bool closeFile();

    virtual void flush() const = 0;
    virtual bool exists() const = 0;

    bool isDirectory() const;
    bool isFile() const;

    String getFileName() const;
    String getHostDirectory() const;
    String getFullPath() const;

    // Direct OS specific flags maps directly to OS bits
    void setAdvancedFlags(const uint64& flags) { advancedFlags = flags; }
    // Sharing mode for other handles
    void setSharingMode(const uint8& sharingFlags) { sharingMode = sharingFlags; }
    // How to open the file (Read or write or both)
    void setFileFlags(const uint8& flags);
    // File specific attribute(Encodings and such)
    void setAttributes(const uint32& attribs) { attributes = attribs; }
    // File on open or creation actions
    void setCreationAction(const uint8& creationAction);

    void addAdvancedFlags(const uint64& flags);
    void removeAdvancedFlags(const uint64& flags);

    void addSharingFlags(const uint8& sharingFlags);
    void removeSharingFlags(const uint8& sharingFlags);

    void addFileFlags(const uint8& flags);
    void removeFileFlags(const uint8& flags);

    void addAttributes(const uint32& attribs);
    void removeAttributes(const uint32& attribs);

    virtual uint64 lastWriteTimeStamp() const = 0;
    virtual uint64 fileSize() const = 0;
    virtual uint64 filePointer() const = 0;
    virtual void seekEnd() const = 0;
    virtual void seekBegin() const = 0;
    virtual void seek(const int64& pointer) const = 0;
    virtual void offsetCursor(const int64& offset) const = 0;

    virtual void read(std::vector<uint8>& readTo, const uint32& bytesToRead = (~0u)) const = 0;
    virtual void write(const ArrayView<uint8>& writeBytes) const = 0;

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

    virtual bool createDirectory() const =0;
};

namespace std 
{
    template<>
    struct PROGRAMCORE_EXPORT default_delete<GenericFile>
    {
        void operator()(GenericFile* _Ptr) const noexcept
        {
            _Ptr->closeFile();
            delete _Ptr;
        }
    };
}
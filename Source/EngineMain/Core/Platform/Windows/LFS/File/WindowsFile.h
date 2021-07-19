#pragma once
#include "../../../LFS/File/GenericFile.h"

class WindowsFile final : public GenericFile {

public:
    WindowsFile(const String& path = ""):GenericFile(path){}
    WindowsFile(WindowsFile&& otherFile);
    WindowsFile(const WindowsFile& otherFile);
    ~WindowsFile();
    void operator=(const WindowsFile& otherFile);
    void operator=(WindowsFile&& otherFile);

    void flush() const override;
    bool exists() const override;

    uint64 lastWriteTimeStamp() const override;
    uint64 fileSize() const override;
    uint64 filePointer() const override;
    void seekEnd() const override;
    void seekBegin() const override;
    void seek(const int64& pointer) const override;
    void offsetCursor(const int64& offset) const override;

    void read(std::vector<uint8>& readTo, const uint32& bytesToRead = (~0u)) const override;
    void write(const std::vector<uint8>& writeBytes) const override;
    void write(const uint8* writeBytes, uint64 count) const override;

    bool deleteFile() override;
    bool renameFile(String newName) override;

    bool createDirectory() const override;

protected:
    virtual GenericFileHandle* openOrCreateImpl() override;
    virtual GenericFileHandle* openImpl()  const override;
    virtual bool closeImpl() const override;

    bool dirDelete() const override;
    bool dirClearAndDelete() const override;

};

namespace LFS {
    typedef WindowsFile PlatformFile;
}
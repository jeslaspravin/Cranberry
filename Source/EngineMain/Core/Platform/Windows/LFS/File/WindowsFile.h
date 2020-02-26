#pragma once
#include "../../../LFS/File/GenericFile.h"

class WindowsFile : public GenericFile {

public:
    WindowsFile(const String& path):GenericFile(path){}
    WindowsFile(WindowsFile&& otherFile);
    WindowsFile(const WindowsFile& otherFile);

    void flush() override;
    bool exists() override;

    uint64 lastWriteTimeStamp() override;
    uint64 fileSize() override;
    uint64 filePointer() override;
    void seekEnd() override;
    void seekBegin() override;
    void seek(const int64& pointer) override;
    void offsetCursor(const int64& offset) override;

    void read(std::vector<uint8>& readTo, const uint32& bytesToRead) override;
    void write(const std::vector<uint8>& writeBytes) override;

    bool deleteFile() override;
    bool renameFile(String newName) override;

    bool createDirectory() override;



protected:
    virtual GenericFileHandle* openOrCreateImpl() override;
    virtual GenericFileHandle* openImpl() override;
    virtual bool closeImpl() override;

    bool dirDelete() override;
    bool dirClearAndDelete() override;

};

namespace LFS {
    typedef WindowsFile PlatformFile;
}
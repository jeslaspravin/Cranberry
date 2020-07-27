#pragma once
#include "../../../LFS/File/GenericFileHandle.h"
#include "../../../../String/String.h"
class WindowsFileHandle final : public GenericFileHandle
{
private:
    void* fileHandle=nullptr;

protected:
    void* getFileHandle() override;

public:
    WindowsFileHandle(uint8 flags, uint8 sharing = (uint8)EFileSharing::ReadOnly,
        uint32 extraFlags = 0, uint64 advancedFlags = 0);

    ~WindowsFileHandle();

    bool openFile(const String& filePath);
    bool closeFile();
};
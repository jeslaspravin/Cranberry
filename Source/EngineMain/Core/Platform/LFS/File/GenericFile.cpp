#include "GenericFile.h"
#include "GenericFileHandle.h"
#include "../../PlatformFunctions.h"
#include "../../../Logger/Logger.h"

void* GenericFile::getFileHandleRaw()
{
    return fileHandle?fileHandle->getFileHandle():nullptr;
}


void GenericFile::setPaths(const String& fPath)
{
    size_t hostDirectoryAt;
    String pathSeperator;
    if (fPath.findAny(hostDirectoryAt, pathSeperator, { "/","\\" }, 0, true))
    {
        directoryPath = { fPath.substr(0, hostDirectoryAt ) };
        fileName = { fPath.substr(hostDirectoryAt + 1) };

        if (fileName.rfind('.') == String::npos)
        {
            fileName = "";
        }
        fullPath = fPath;
    }
    else
    {
        Logger::error("File", "%s() : File path \"%s\" is invalid", __func__, fPath.getChar());
    }
}

GenericFile::GenericFile():
    fileHandle(nullptr),
    fullPath(),
    directoryPath(),
    fileName()
{

}

GenericFile::GenericFile(const String& path) :
    fileHandle(nullptr)
{
    setPaths(path);
}

GenericFile::~GenericFile()
{
    closeFile();
}

bool GenericFile::openOrCreate()
{
    if (fileHandle)
    {
        return false;
    }
    fileHandle = openOrCreateImpl();
    return fileHandle != nullptr;
}

bool GenericFile::openFile()
{
    if (fileHandle)
    {
        return false;
    }
    fileHandle = openImpl();
    return fileHandle != nullptr;
}

bool GenericFile::closeFile()
{
    if (fileHandle && closeImpl())
    {
        delete fileHandle;
        fileHandle = nullptr;
        return true;
    }
    return false;
}

bool GenericFile::isDirectory()
{
    return fileName.empty();
}

bool GenericFile::isFile()
{
    return !isDirectory();
}

String GenericFile::getFileName()
{
    return fileName;
}

String GenericFile::getHostDirectory()
{
    return directoryPath;
}

String GenericFile::getFullPath()
{
    return fullPath;
}

void GenericFile::setFileFlags(const uint8& flags)
{
    uint8 accessFlags = flags & FileFlags::ACCESS_FLAGS;
    uint8 actionFlags = flags & FileFlags::OPEN_ACTION_FLAGS;

    if (!ONE_BIT_SET(actionFlags)) {
        actionFlags = fileFlags & FileFlags::OPEN_ACTION_FLAGS;
    }

    fileFlags = accessFlags | actionFlags;
}

void GenericFile::setCreationAction(const uint8& creationAction)
{
    uint8 actionFlags = creationAction & FileFlags::OPEN_ACTION_FLAGS;

    if (!ONE_BIT_SET(actionFlags)) {
        actionFlags = fileFlags & FileFlags::OPEN_ACTION_FLAGS;
    }

    fileFlags |= actionFlags;
}

void GenericFile::addAdvancedFlags(const uint64& flags)
{
    advancedFlags |= flags;
}

void GenericFile::removeAdvancedFlags(const uint64& flags)
{
    advancedFlags &= ~flags;
}

void GenericFile::addSharingFlags(const uint8& sharingFlags)
{
    sharingMode |= sharingFlags;
}

void GenericFile::removeSharingFlags(const uint8& sharingFlags)
{
    sharingMode &= ~sharingFlags;
}

void GenericFile::addFileFlags(const uint8& flags)
{

    uint8 accessFlags = flags & FileFlags::ACCESS_FLAGS;
    uint8 actionFlags = flags & FileFlags::OPEN_ACTION_FLAGS;

    if (ONE_BIT_SET(actionFlags)){
        removeFileFlags(FileFlags::OPEN_ACTION_FLAGS);
    }
    else {
        actionFlags = fileFlags & FileFlags::OPEN_ACTION_FLAGS;
    }

    fileFlags |= accessFlags | actionFlags;
}

void GenericFile::removeFileFlags(const uint8& flags)
{
    fileFlags &= ~flags;
}

void GenericFile::addAttributes(const uint32& attribs)
{
    attributes |= attribs;
}

void GenericFile::removeAttributes(const uint32& attribs)
{
    attributes &= ~attribs;
}

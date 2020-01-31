#pragma once
#include <string>
#include "../../../String/String.h"
#include <memory>

class GenericFileHandle;

class GenericFile {
	
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

	void* getFileHandleRaw();
	GenericFileHandle* getFileHandle() { return fileHandle; }

	virtual GenericFileHandle* openOrCreateImpl()=0;
	virtual GenericFileHandle* openImpl() = 0;
	// Must flush if necessary
	virtual bool closeImpl() = 0;
	virtual bool dirDelete() = 0;
	virtual bool dirClearAndDelete() = 0;

	void setPaths(const String& fPath);

public:
	GenericFile();
	GenericFile(const String& path);

	virtual ~GenericFile();

	// Opens only if previous file is properly closed
	bool openOrCreate();
	bool openFile();
	// Closes the file if it exists 
	bool closeFile();

	virtual void flush() = 0;
	virtual bool exists() = 0;

	bool isDirectory();
	bool isFile();

	String getFileName();
	String getHostDirectory();
	String getFullPath();

	void setAdvancedFlags(const uint64& flags) { advancedFlags = flags; }
	void setSharingMode(const uint8& sharingFlags) { sharingMode = sharingFlags; }
	void setFileFlags(const uint8& flags);
	void setAttributes(const uint32& attribs) { attributes = attribs; }
	void setCreationAction(const uint8& creationAction);

	void addAdvancedFlags(const uint64& flags);
	void removeAdvancedFlags(const uint64& flags);

	void addSharingFlags(const uint8& sharingFlags);
	void removeSharingFlags(const uint8& sharingFlags);

	void addFileFlags(const uint8& flags);
	void removeFileFlags(const uint8& flags);

	void addAttributes(const uint32& attribs);
	void removeAttributes(const uint32& attribs);

	virtual uint64 lastWriteTimeStamp() = 0;
	virtual uint64 fileSize() = 0;
	virtual uint64 filePointer() = 0;
	virtual void seekEnd() = 0;
	virtual void seekBegin() = 0;
	virtual void seek(const int64& pointer) = 0;
	virtual void offsetCursor(const int64& offset) = 0;

	virtual void read(std::vector<uint8>& readTo, const uint32& bytesToRead) = 0;
	virtual void write(const std::vector<uint8>& writeBytes) = 0;

	virtual bool deleteFile() = 0;
	virtual bool renameFile(String newName) = 0;

	// Works only if directory
	template <bool ClearFiles>
	bool deleteDirectory();

	template <>
	bool deleteDirectory<true>()
	{
		return dirClearAndDelete();
	}

	template <>
	bool deleteDirectory<false>()
	{
		return dirDelete();
	}

	virtual bool createDirectory()=0;


};

namespace std {
	template<>
	struct default_delete<GenericFile>
	{
		void operator()(GenericFile* _Ptr) const noexcept {
			_Ptr->closeFile();
			delete _Ptr;
		}
	};
}
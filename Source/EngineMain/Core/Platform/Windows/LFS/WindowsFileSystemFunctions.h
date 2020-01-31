#pragma once

#include "../../LFS/GenericFileSystemFunctions.h"

class WindowsFileSystemFunctions : public GenericFileSystemFunctions<WindowsFileSystemFunctions>
{
public:

	static String applicationDirectory(String &appName);
	static bool moveFile(GenericFile* moveFrom, GenericFile* moveTo);
	static bool copyFile(GenericFile* copyFrom, GenericFile* copyTo);
	static bool replaceFile(GenericFile* replaceWith, GenericFile* replacing, GenericFile* backupFile);

};

namespace LFS {
	typedef GenericFileSystemFunctions<WindowsFileSystemFunctions> FileSystemFunctions;
}
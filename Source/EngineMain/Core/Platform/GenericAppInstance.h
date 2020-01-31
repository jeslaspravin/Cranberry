#pragma once

#include "../Platform/PlatformTypes.h"
#include "../String/String.h"

class GenericAppWindow;


struct GenericAppInstance {

	String applicationName;
	int32 headVersion;
	int32 majorVersion;
	int32 subVersion;
	String cmdLine;

	GenericAppWindow* appWindow;
};
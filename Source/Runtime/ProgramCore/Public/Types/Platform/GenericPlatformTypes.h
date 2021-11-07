#pragma once

#include "ProgramCoreExports.h"

struct PROGRAMCORE_EXPORT LibPointer 
{

    virtual ~LibPointer() = default;

};

typedef LibPointer* LibPointerPtr;

struct PROGRAMCORE_EXPORT PlatformInstance{};


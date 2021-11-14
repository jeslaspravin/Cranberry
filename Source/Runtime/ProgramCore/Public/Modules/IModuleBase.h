#pragma once

#include "ProgramCoreExports.h"

// Module life cycle interface
class PROGRAMCORE_EXPORT IModuleBase
{
public:
    virtual ~IModuleBase() = default;

    virtual void init() = 0;
    virtual void release() = 0;
};

class PROGRAMCORE_EXPORT ModuleNoImpl : public IModuleBase
{
public:
    void init() final {}
    void release() final {}
};

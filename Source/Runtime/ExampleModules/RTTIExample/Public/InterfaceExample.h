/*!
 * \file InterfaceExample.h
 *
 * \author Jeslas
 * \date April 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "RTTIExampleExports.h"
#include "ReflectionMacros.h"
#include "InterfaceExample.gen.h"

class META_ANNOTATE_API(RTTIEXAMPLE_EXPORT, Interface) IInterfaceExample
{
    GENERATED_INTERFACE_CODES()
public:
    virtual void exampleFunc() const = 0;
};

class META_ANNOTATE_API(RTTIEXAMPLE_EXPORT, Interface) IInterfaceExample2
{
    GENERATED_INTERFACE_CODES()
public:
    virtual void exampleFunc() const = 0;
};
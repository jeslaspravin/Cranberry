/*!
 * \file WindowsAppInstance.cpp
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "WindowsAppInstance.h"

InstanceHandle WindowsAppInstance::getPlatformAppInstance() const { return windowsInstance; }

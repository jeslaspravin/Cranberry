/*!
 * \file EngineCoreModule.h
 *
 * \author Jeslas
 * \date June 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "IEngineCoreModule.h"

class EngineCoreModule final : public IEngineCore
{
public:
    void init() final;
    void release() final;
};
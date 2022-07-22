/*!
 * \file CranberryEngineApp.h
 *
 * \author Jeslas
 * \date July 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "ApplicationInstance.h"

class CranberryEngineApp final : public ApplicationInstance
{
public:
    CranberryEngineApp(const AppInstanceCreateInfo &ci)
        : ApplicationInstance(ci)
    {}

    /* ApplicationInstance overrides */
    void onStart() final;
    void onTick() final;
    void onExit() final;
    void onRendererStateEvent(ERenderStateEvent state) final;

    /* Override ends */
};
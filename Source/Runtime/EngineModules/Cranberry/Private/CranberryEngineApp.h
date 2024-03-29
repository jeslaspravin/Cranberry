/*!
 * \file CranberryEngineApp.h
 *
 * \author Jeslas
 * \date July 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "ApplicationInstance.h"
#include "GCReferenceCollector.h"

class IReflectionRuntimeModule;
class ICoreObjectsModule;

class CranberryEngineApp final : public ApplicationInstance
{
public:
    CranberryEngineApp(const AppInstanceCreateInfo &ci)
        : ApplicationInstance(ci)
        , rttiModule(nullptr)
        , coreObjModule(nullptr)
    {}

    /* ApplicationInstance overrides */
    void onStart() final;
    void onTick() final;
    void onExit() final;
    void onRendererStateEvent(ERenderStateEvent state) final;

    /* Override ends */

private:
    IReflectionRuntimeModule *rttiModule;
    ICoreObjectsModule *coreObjModule;
};
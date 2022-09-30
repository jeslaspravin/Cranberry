/*!
 * \file EngineBase.cpp
 *
 * \author Jeslas
 * \date July 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#include "Classes/EngineBase.h"
#include "Classes/WorldsManager.h"
#include "IApplicationModule.h"
#include "ApplicationInstance.h"

cbe::EngineBase *gCBEEngine = nullptr;

namespace cbe
{
EngineBase::EngineBase()
{
    if (BIT_NOT_SET(getFlags(), EObjectFlagBits::ObjFlag_Default))
    {
        debugAssert(gCBEEngine == nullptr);
        gCBEEngine = this;
    }
}

void EngineBase::onStart()
{
    // TODO(Jeslas) :
    engineStart();
}

void EngineBase::onTick()
{
    // TODO(Jeslas) :
    engineTick(IApplicationModule::get()->getApplication()->timeData.deltaTime);
}

void EngineBase::onExit()
{
    // TODO(Jeslas) :
    engineExit();
}

WorldsManager *EngineBase::worldManager() const { return cbe::getDefaultObject<WorldsManager>(); }

} // namespace cbe

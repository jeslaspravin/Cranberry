/*!
 * \file ApplicationModule.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "IApplicationModule.h"
#include "IRenderInterfaceModule.h"
#include "ApplicationInstance.h"
#include "WindowManager.h"

class ApplicationModule final : public IApplicationModule
{
private:
    ApplicationInstance appInstance;
    WindowManager windowMan;
public:
    AppWindowDelegate onWindowCreated;
    // Called just before windows is finalized to be destroyed
    AppWindowDelegate onWindowDestroyed;
    // Called before window property change has lead to surface reinitialization
    AppWindowDelegate preWindowSurfaceUpdate;
    // When resized/updated that lead to underlying surface/canvas to be updated
    AppWindowDelegate onWindowSurfaceUpdated;
    SimpleDelegate onAllWindowsDestroyed;
private:
    DelegateHandle graphicsInitEventHandle;
    void graphicsInitEvents(ERenderStateEvent renderState);
public:

    /* IApplicationModule finals */
    void createApplication(const AppInstanceCreateInfo& appCI) final;
    const ApplicationInstance* getApplication() const final;
    GenericAppWindow* mainWindow() final;
    WindowManager* getWindowManager() final;
    bool pollWindows() final;

    DelegateHandle registerOnWindowCreated(AppWindowDelegate::SingleCastDelegateType callback) final;
    void unregisterOnWindowCreated(const DelegateHandle& callbackHandle) final;
    DelegateHandle registerPreWindowSurfaceUpdate(AppWindowDelegate::SingleCastDelegateType callback) final;
    void unregisterPreWindowSurfaceUpdate(const DelegateHandle& callbackHandle) final;
    DelegateHandle registerOnWindowSurfaceUpdated(AppWindowDelegate::SingleCastDelegateType callback) final;
    void unregisterOnWindowSurfaceUpdated(const DelegateHandle& callbackHandle) final;
    DelegateHandle registerOnWindowDestroyed(AppWindowDelegate::SingleCastDelegateType callback) final;
    void unregisterOnWindowDestroyed(const DelegateHandle& callbackHandle) final;
    DelegateHandle registerAllWindowDestroyed(SimpleDelegate::SingleCastDelegateType callback) final;
    void unregisterAllWindowDestroyed(const DelegateHandle& callbackHandle) final;
    /* IModuleBase finals */
    void init() final;
    void release() final;
    /* finals end */

};
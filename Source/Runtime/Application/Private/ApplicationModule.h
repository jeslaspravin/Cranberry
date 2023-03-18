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
#include "WindowManager.h"
#include "InputSystem/InputSystem.h"

class ApplicationInstance;

class ApplicationModule final : public IApplicationModule
{
private:
    ApplicationInstance *appInstance;
    WindowManager windowMan;
    InputSystem inputSystem;

    AppWindowDelegate onWindowCreated;
    // Called just before windows is finalized to be destroyed
    AppWindowDelegate onWindowDestroyed;
    // Called before window property change has lead to surface reinitialization
    AppWindowDelegate onPreWindowSurfaceUpdate;
    // When resized/updated that lead to underlying surface/canvas to be updated
    AppWindowDelegate onWindowSurfaceUpdated;
    SimpleDelegate onAllWindowsDestroyed;

public:
    /* IApplicationModule finals */
    ApplicationInstance *getApplication() const final;

    void windowCreated(GenericAppWindow *createdWindow) const final;
    DelegateHandle registerOnWindowCreated(AppWindowDelegate::SingleCastDelegateType callback) final;
    void unregisterOnWindowCreated(const DelegateHandle &callbackHandle) final;

    void preWindowSurfaceUpdate(GenericAppWindow *window) const final { onPreWindowSurfaceUpdate.invoke(window); }
    DelegateHandle registerPreWindowSurfaceUpdate(AppWindowDelegate::SingleCastDelegateType callback) final;
    void unregisterPreWindowSurfaceUpdate(const DelegateHandle &callbackHandle) final;

    void windowSurfaceUpdated(GenericAppWindow *window) const final { onWindowSurfaceUpdated.invoke(window); }
    DelegateHandle registerOnWindowSurfaceUpdated(AppWindowDelegate::SingleCastDelegateType callback) final;
    void unregisterOnWindowSurfaceUpdated(const DelegateHandle &callbackHandle) final;

    void windowDestroyed(GenericAppWindow *window) const final { onWindowDestroyed.invoke(window); }
    DelegateHandle registerOnWindowDestroyed(AppWindowDelegate::SingleCastDelegateType callback) final;
    void unregisterOnWindowDestroyed(const DelegateHandle &callbackHandle) final;

    void allWindowDestroyed() const final;
    DelegateHandle registerAllWindowDestroyed(SimpleDelegate::SingleCastDelegateType callback) final;
    void unregisterAllWindowDestroyed(const DelegateHandle &callbackHandle) final;

    /* IModuleBase finals */
    void init() final;
    void release() final;
    /* finals end */
protected:
    void startAndRun(ApplicationInstance *appInst, const AppInstanceCreateInfo &appCI) final;

private:
    DelegateHandle graphicsInitEventHandle;
    void graphicsInitEvents(ERenderStateEvent renderState);

    static uint32 getThreadingConstraints();
};
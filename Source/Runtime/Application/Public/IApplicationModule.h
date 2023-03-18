/*!
 * \file IApplicationModule.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "ApplicationExports.h"
#include "Modules/IModuleBase.h"
#include "Types/Containers/ReferenceCountPtr.h"
#include "Types/Delegates/Delegate.h"

class ApplicationInstance;
class GenericAppWindow;
struct AppInstanceCreateInfo;
class WindowManager;

using AppWindowDelegate = Delegate<GenericAppWindow *>;

class IApplicationModule : public IModuleBase
{
protected:
    virtual void startAndRun(ApplicationInstance *appInst, const AppInstanceCreateInfo &appCI) = 0;

public:
    APPLICATION_EXPORT static IApplicationModule *get();

    template <typename ApplicationType>
    requires std::is_base_of_v<ApplicationInstance, ApplicationType>
    void startApplication(const AppInstanceCreateInfo &appCI)
    {
        ApplicationType appInstance(appCI);
        startAndRun(&appInstance, appCI);
    }

    APPLICATION_EXPORT virtual ApplicationInstance *getApplication() const = 0;

    virtual void windowCreated(GenericAppWindow *createdWindow) const = 0;
    APPLICATION_EXPORT virtual DelegateHandle registerOnWindowCreated(AppWindowDelegate::SingleCastDelegateType callback) = 0;
    APPLICATION_EXPORT virtual void unregisterOnWindowCreated(const DelegateHandle &callbackHandle) = 0;

    // Called before window property change has lead to surface reinitialization
    virtual void preWindowSurfaceUpdate(GenericAppWindow *window) const = 0;
    APPLICATION_EXPORT virtual DelegateHandle registerPreWindowSurfaceUpdate(AppWindowDelegate::SingleCastDelegateType callback) = 0;
    APPLICATION_EXPORT virtual void unregisterPreWindowSurfaceUpdate(const DelegateHandle &callbackHandle) = 0;

    // When resized/updated that lead to underlying surface/canvas to be updated
    virtual void windowSurfaceUpdated(GenericAppWindow *window) const = 0;
    APPLICATION_EXPORT virtual DelegateHandle registerOnWindowSurfaceUpdated(AppWindowDelegate::SingleCastDelegateType callback) = 0;
    APPLICATION_EXPORT virtual void unregisterOnWindowSurfaceUpdated(const DelegateHandle &callbackHandle) = 0;

    // Called just before windows is finalized to be0 destroyed
    virtual void windowDestroyed(GenericAppWindow *window) const = 0;
    APPLICATION_EXPORT virtual DelegateHandle registerOnWindowDestroyed(AppWindowDelegate::SingleCastDelegateType callback) = 0;
    APPLICATION_EXPORT virtual void unregisterOnWindowDestroyed(const DelegateHandle &callbackHandle) = 0;

    // After all windows are destroyed
    virtual void allWindowDestroyed() const = 0;
    APPLICATION_EXPORT virtual DelegateHandle registerAllWindowDestroyed(SimpleDelegate::SingleCastDelegateType callback) = 0;
    APPLICATION_EXPORT virtual void unregisterAllWindowDestroyed(const DelegateHandle &callbackHandle) = 0;
};

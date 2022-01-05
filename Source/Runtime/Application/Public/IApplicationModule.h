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

#include "Types/Delegates/Delegate.h"
#include "Modules/IModuleBase.h"
#include "ApplicationExports.h"
#include "Types/Containers/ReferenceCountPtr.h"

class ApplicationInstance;
class GenericAppWindow;
struct AppInstanceCreateInfo;
class WindowManager;

using AppWindowDelegate = Delegate<GenericAppWindow*>;

class APPLICATION_EXPORT IApplicationModule : public IModuleBase
{
public:
    static IApplicationModule* get();

    virtual void createApplication(const AppInstanceCreateInfo& appCI) = 0;
    virtual const ApplicationInstance* getApplication() const = 0;
    virtual GenericAppWindow* mainWindow() = 0;
    virtual WindowManager* getWindowManager() = 0;
    virtual bool pollWindows() = 0;

    virtual DelegateHandle registerOnWindowCreated(AppWindowDelegate::SingleCastDelegateType callback) = 0;
    virtual void unregisterOnWindowCreated(const DelegateHandle& callbackHandle) = 0;

    // Called before window property change has lead to surface reinitialization
    virtual DelegateHandle registerPreWindowSurfaceUpdate(AppWindowDelegate::SingleCastDelegateType callback) = 0;
    virtual void unregisterPreWindowSurfaceUpdate(const DelegateHandle& callbackHandle) = 0;

    // When resized/updated that lead to underlying surface/canvas to be updated
    virtual DelegateHandle registerOnWindowSurfaceUpdated(AppWindowDelegate::SingleCastDelegateType callback) = 0;
    virtual void unregisterOnWindowSurfaceUpdated(const DelegateHandle& callbackHandle) = 0;

    // Called just before windows is finalized to be destroyed
    virtual DelegateHandle registerOnWindowDestroyed(AppWindowDelegate::SingleCastDelegateType callback) = 0;
    virtual void unregisterOnWindowDestroyed(const DelegateHandle& callbackHandle) = 0;

    // After all windows are destroyed
    virtual DelegateHandle registerAllWindowDestroyed(SimpleDelegate::SingleCastDelegateType callback) = 0;
    virtual void unregisterAllWindowDestroyed(const DelegateHandle& callbackHandle) = 0;
};

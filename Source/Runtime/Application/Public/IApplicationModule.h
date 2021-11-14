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

    virtual DelegateHandle registerPreWindowSurfaceUpdate(AppWindowDelegate::SingleCastDelegateType callback) = 0;
    virtual void unregisterPreWindowSurfaceUpdate(const DelegateHandle& callbackHandle) = 0;

    virtual DelegateHandle registerOnWindowSurfaceUpdated(AppWindowDelegate::SingleCastDelegateType callback) = 0;
    virtual void unregisterOnWindowSurfaceUpdated(const DelegateHandle& callbackHandle) = 0;

    virtual DelegateHandle registerOnWindowDestroyed(AppWindowDelegate::SingleCastDelegateType callback) = 0;
    virtual void unregisterOnWindowDestroyed(const DelegateHandle& callbackHandle) = 0;

    virtual DelegateHandle registerAllWindowDestroyed(SimpleDelegate::SingleCastDelegateType callback) = 0;
    virtual void unregisterAllWindowDestroyed(const DelegateHandle& callbackHandle) = 0;
};

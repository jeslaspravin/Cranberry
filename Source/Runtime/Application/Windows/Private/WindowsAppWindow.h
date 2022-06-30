/*!
 * \file WindowsAppWindow.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "GenericAppWindow.h"

class WindowsAppWindow final : public GenericAppWindow
{

private:
    WindowHandle windowsHandle;

protected:
    // void resizeWindow() override;

public:
    void createWindow(const ApplicationInstance *appInstance) override;
    void updateWindow() override;
    void destroyWindow() override;
    bool isValidWindow() const override;
    WindowHandle getWindowHandle() const override { return windowsHandle; }

    void pushEvent(uint32 eventType, LambdaFunction<void> &&function);
    void activateWindow() const;
    void deactivateWindow() const;
    void windowResizing(uint32 width, uint32 height) const;
    void windowDpiChanged(uint32 newDpi);
    void windowDestroyRequested() const;
    Rect windowClientRect() const override;
};

namespace GPlatformInstances
{
typedef WindowsAppWindow PlatformAppWindow;
}
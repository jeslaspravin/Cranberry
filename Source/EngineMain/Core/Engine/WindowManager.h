#pragma once
#include <map>

class GenericAppWindow;
class GenericWindowCanvas;

class WindowManager final
{
private:
    GenericAppWindow* appMainWindow = nullptr;

    struct ManagerData
    {
        GenericWindowCanvas* windowCanvas = nullptr;
    };

    std::map<GenericAppWindow*, ManagerData> windowsOpened;

public:

    GenericAppWindow* getMainWindow() const;
    GenericWindowCanvas* getWindowCanvas(GenericAppWindow* window) const;

    void initMain();
    // One time only function
    void postInitGraphicCore();
    void destroyMain();
};


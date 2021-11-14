#pragma once

class ApplicationInstance;

template <typename ...Parameters>
struct PFN_SurfaceKHR {

public:
    virtual void setInstanceWindow(const ApplicationInstance* instance,const class GenericAppWindow* window) = 0;
    virtual void operator()(Parameters ...params) const = 0;
};
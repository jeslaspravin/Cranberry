#pragma once

template <typename ...Parameters>
struct PFN_SurfaceKHR {
	virtual void operator()(Parameters ...params) const = 0;
};
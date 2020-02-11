#pragma once
#include "../Core/Memory/SmartPointers.h"
#include "../Core/Platform/PlatformTypes.h"


namespace std
{
	template <class _Ty>
	class allocator;

	template <class _Ty, class _Alloc = allocator<_Ty>>
	class vector;
}

template<typename HelperType>
class GraphicsHelperAPI
{
private:
	GraphicsHelperAPI() {}

public:
	
	static SharedPtr<class GraphicsSemaphore> createSemaphore(class IGraphicsInstance* graphicsInstance)
	{
		return HelperType::createSemaphore(graphicsInstance);
	}

	static SharedPtr<class GraphicsTimelineSemaphore> createTimelineSemaphore(class IGraphicsInstance* graphicsInstance)
	{
		return HelperType::createTimelineSemaphore(graphicsInstance);
	}

	static void waitTimelineSemaphores(class IGraphicsInstance* graphicsInstance,
		std::vector<SharedPtr<class GraphicsTimelineSemaphore>>* semaphores, std::vector<uint64>* waitForValues)
	{
		HelperType::waitTimelineSemaphores(graphicsInstance, semaphores,waitForValues);
	}

	static SharedPtr<class GraphicsFence> createFence(class IGraphicsInstance* graphicsInstance)
	{
		return HelperType::createFence(graphicsInstance);
	}

	static void waitFences(class IGraphicsInstance* graphicsInstance,
		std::vector<SharedPtr<class GraphicsFence>>* fences, bool waitAll)
	{
		HelperType::waitFences(graphicsInstance, fences,waitAll);
	}
};

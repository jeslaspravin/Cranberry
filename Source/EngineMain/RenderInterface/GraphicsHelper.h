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
    
    static SharedPtr<class GraphicsSemaphore> createSemaphore(class IGraphicsInstance* graphicsInstance, const char* semaphoreName)
    {
        return HelperType::createSemaphore(graphicsInstance,semaphoreName);
    }

    static SharedPtr<class GraphicsTimelineSemaphore> createTimelineSemaphore(class IGraphicsInstance* graphicsInstance,
        const char* semaphoreName)
    {
        return HelperType::createTimelineSemaphore(graphicsInstance,semaphoreName);
    }

    static void waitTimelineSemaphores(class IGraphicsInstance* graphicsInstance,
        std::vector<SharedPtr<class GraphicsTimelineSemaphore>>* semaphores, std::vector<uint64>* waitForValues)
    {
        HelperType::waitTimelineSemaphores(graphicsInstance, semaphores,waitForValues);
    }

    static SharedPtr<class GraphicsFence> createFence(class IGraphicsInstance* graphicsInstance,const char* fenceName)
    {
        return HelperType::createFence(graphicsInstance,fenceName);
    }

    static void waitFences(class IGraphicsInstance* graphicsInstance,
        std::vector<SharedPtr<class GraphicsFence>>* fences, bool waitAll)
    {
        HelperType::waitFences(graphicsInstance, fences,waitAll);
    }

    static void presentImage(class IGraphicsInstance* graphicsInstance, std::vector<class GenericWindowCanvas*>* canvases,
        std::vector<uint32>* imageIndex, std::vector<SharedPtr<class GraphicsSemaphore>>* waitOnSemaphores)
    {
        HelperType::presentImage(graphicsInstance, canvases,imageIndex,waitOnSemaphores);
    }
};

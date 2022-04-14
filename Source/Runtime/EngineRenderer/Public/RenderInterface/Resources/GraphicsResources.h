/*!
 * \file GraphicsResources.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once
#include "EngineRendererExports.h"
#include "Memory/SmartPointers.h"
#include "String/String.h"

#include <forward_list>
#include <vector>

class GraphicsResource;
class GraphicsResourceType;

/* This graph is not yet thread safe */
class ResourceTypesGraph
{
public:
    struct TypeNode
    {
        const GraphicsResourceType *type;
        std::vector<TypeNode> childs;

        bool isLeaf() { return childs.empty(); }
    };

private:
    TypeNode root;
    std::vector<const GraphicsResourceType *> insertWaitQueue;
    void insertType(const GraphicsResourceType *type, TypeNode *fromNode = nullptr);
    void graphAllChilds(TypeNode *fromNode, std::vector<const GraphicsResourceType *> &outChilds,
        bool bRecursively) const;
    void graphAllLeafChilds(TypeNode *fromNode, std::vector<const GraphicsResourceType *> &outChilds,
        bool bRecursively) const;

public:
    // Insert happens only when querying for some resource
    void lazyInsert(const GraphicsResourceType *type);
    void findChildsOf(const GraphicsResourceType *type,
        std::vector<const GraphicsResourceType *> &outChilds, bool bRecursively = false,
        bool bOnlyLeafChilds = false);
};

class ENGINERENDERER_EXPORT GraphicsResourceType
{

public:
    typedef void (*DeleteFn)(GraphicsResource *);

private:
    String typeName;
    GraphicsResource *defaultResource = nullptr;

    using GraphicsResourceList = std::forward_list<GraphicsResource *>;
    GraphicsResourceList registeredResources;

    DeleteFn deleteResource;
    ResourceTypesGraph &getTypeGraph() const;

protected:
    virtual bool verifyParent(const GraphicsResourceType *otherType) const = 0;

    GraphicsResourceType(GraphicsResource *resource, DeleteFn deleteFunc, const String &resTypeName);
    virtual ~GraphicsResourceType();

public:
    GraphicsResourceType(const GraphicsResourceType &other) = delete;
    GraphicsResourceType() = delete;
    void operator=(const GraphicsResourceType &) = delete;
    void operator=(GraphicsResourceType &&) = delete;

    bool operator==(const GraphicsResourceType &otherType) const
    {
        return defaultResource == otherType.defaultResource;
    }
    bool operator!=(const GraphicsResourceType &otherType) const
    {
        return defaultResource != otherType.defaultResource;
    }

    void registerResource(GraphicsResource *resource);
    void unregisterResource(GraphicsResource *resource);

    GraphicsResource *getDefault() const { return defaultResource; }
    const String &getName() const { return typeName; }
    // Returns all registered resources of this type only, no parent type resources are returned
    void allRegisteredResources(std::vector<GraphicsResource *> &outResources, bool bRecursively = false,
        bool bOnlyLeaf = false) const;
    void allChildDefaultResources(std::vector<GraphicsResource *> &outResources,
        bool bRecursively = false, bool bOnlyLeaf = false) const;

    bool isChildOf(const GraphicsResourceType *otherType) const;
    virtual const GraphicsResourceType *getParent() const = 0;
    virtual bool isRootType() const = 0;

    template <typename CheckType>
    constexpr bool isChildOf() const
    {
        return this == CheckType::staticType() || verifyParent(CheckType::staticType());
    }
};

template <typename Parent, typename Child>
class GraphicsResourceTypeSpecialized final : public GraphicsResourceType
{
protected:
    using ThisType = typename Child;
    using ParentType = typename Parent;

    constexpr bool verifyParent(const GraphicsResourceType *otherType) const override
    {
        return !isRootType() && ParentType::staticType()->isChildOf(otherType);
    }

public:
    GraphicsResourceTypeSpecialized(
        GraphicsResource *resource, GraphicsResourceType::DeleteFn deleteFunc, const String &resTypeName)
        : GraphicsResourceType(resource, deleteFunc, resTypeName)
    {}

    const GraphicsResourceType *getParent() const override
    {
        return isRootType() ? nullptr : ParentType::staticType();
    }

    bool isRootType() const override { return ThisType::staticType() == ParentType::staticType(); }
};

#ifndef DECLARE_GRAPHICS_RESOURCE
#define DECLARE_GRAPHICS_RESOURCE(NewTypeName, NewTypeTemplates, BaseTypeName, BaseTypeTemplates)       \
private:                                                                                                \
    using NewType = NewTypeName##NewTypeTemplates;                                                      \
    using BaseType = BaseTypeName##BaseTypeTemplates;                                                   \
                                                                                                        \
protected:                                                                                              \
    using NewTypeName##_Type = GraphicsResourceTypeSpecialized<typename BaseType, typename NewType>;    \
    static UniquePtr<typename NewTypeName##_Type> STATIC_TYPE;                                          \
    virtual GraphicsResourceType *privateType() const;                                                  \
                                                                                                        \
public:                                                                                                 \
    virtual const GraphicsResourceType *getType() const;                                                \
    static const GraphicsResourceType *staticType();                                                    \
                                                                                                        \
private:                                                                                                \
    static void delFn(GraphicsResource *resource);                                                      \
                                                                                                        \
protected:                                                                                              \
    template <typename CheckType, typename BaseType>                                                    \
    struct TypeOfBase                                                                                   \
    {                                                                                                   \
                                                                                                        \
        static bool isTypeOfBase() { return BaseType::template isTypeOf<CheckType>(); }                 \
    };                                                                                                  \
                                                                                                        \
    template <typename CheckType>                                                                       \
    struct TypeOfBase<CheckType, GraphicsResource>                                                      \
    {                                                                                                   \
                                                                                                        \
        static bool isTypeOfBase() { return false; }                                                    \
    };                                                                                                  \
                                                                                                        \
public:                                                                                                 \
    template <typename CheckType>                                                                       \
    static bool isTypeOf()                                                                              \
    {                                                                                                   \
        return TypeOfBase<CheckType, GraphicsResource>::isTypeOfBase();                                 \
    }                                                                                                   \
                                                                                                        \
    template <>                                                                                         \
    static bool isTypeOf<GraphicsResource>()                                                            \
    {                                                                                                   \
        return true;                                                                                    \
    }
#endif // DECLARE_GRAPHICS_RESOURCE

#ifndef DEFINE_GRAPHICS_RESOURCE
#define DEFINE_GRAPHICS_RESOURCE(NewTypeName)                                                           \
    UniquePtr<NewTypeName::##NewTypeName##_Type> NewTypeName::STATIC_TYPE                               \
        = UniquePtr<NewTypeName::##NewTypeName##_Type>(new NewTypeName::##NewTypeName##_Type(           \
            new NewTypeName(), &NewTypeName::delFn, TCHAR(#NewTypeName)));                              \
                                                                                                        \
    void NewTypeName::delFn(GraphicsResource *resource) { delete resource; }                            \
    GraphicsResourceType *NewTypeName::privateType() const { return NewTypeName::STATIC_TYPE.get(); }   \
    const GraphicsResourceType *NewTypeName::getType() const { return NewTypeName::STATIC_TYPE.get(); } \
    const GraphicsResourceType *NewTypeName::staticType() { return NewTypeName::STATIC_TYPE.get(); }
#endif

#ifndef DEFINE_TEMPLATED_GRAPHICS_RESOURCE
#define DEFINE_TEMPLATED_GRAPHICS_RESOURCE(NewTypeName, NewTypeTemplates, TemplatesDefine)              \
    template##NewTypeTemplates##UniquePtr<                                                              \
        typename NewTypeName##TemplatesDefine## ::##NewTypeName##_Type>                                 \
        NewTypeName##TemplatesDefine## ::STATIC_TYPE                                                    \
        = UniquePtr<typename NewTypeName##TemplatesDefine## ::##NewTypeName##_Type>(                    \
            new typename NewTypeName##TemplatesDefine## ::##NewTypeName##_Type(new                      \
                typename NewTypeName##TemplatesDefine## ::NewType(),                                    \
                &NewTypeName##TemplatesDefine## ::delFn, TCHAR(#NewTypeName #TemplatesDefine)));        \
                                                                                                        \
    template##NewTypeTemplates##void NewTypeName##TemplatesDefine## ::delFn(GraphicsResource *resource) \
    {                                                                                                   \
        delete resource;                                                                                \
    }                                                                                                   \
    template##NewTypeTemplates##GraphicsResourceType *NewTypeName##TemplatesDefine## ::privateType()    \
        const                                                                                           \
    {                                                                                                   \
        return NewTypeName##TemplatesDefine## ::STATIC_TYPE.get();                                      \
    }                                                                                                   \
    template##NewTypeTemplates##const GraphicsResourceType *NewTypeName##TemplatesDefine## ::getType()  \
        const                                                                                           \
    {                                                                                                   \
        return NewTypeName##TemplatesDefine## ::STATIC_TYPE.get();                                      \
    }                                                                                                   \
    template##NewTypeTemplates##const GraphicsResourceType                                              \
        *NewTypeName##TemplatesDefine## ::staticType()                                                  \
    {                                                                                                   \
        return NewTypeName##TemplatesDefine## ::STATIC_TYPE.get();                                      \
    }
#endif // DEFINE_GRAPHICS_RESOURCE

class ENGINERENDERER_EXPORT GraphicsResource
{
    DECLARE_GRAPHICS_RESOURCE(GraphicsResource, , GraphicsResource, )

public:
    GraphicsResource(const GraphicsResource &otherResource) = delete;
    GraphicsResource(GraphicsResource &&otherResource) = delete;

    GraphicsResource() = default;
    virtual ~GraphicsResource() = default;

    // always call parent init and release functions if the resource needs to be registered in collection
    virtual void init();
    virtual void reinitResources();
    virtual void release();
    virtual String getResourceName() const;
    // This needs to be set before initializing or needs be reinit for the GPU resource be relabeled.
    virtual void setResourceName(const String &name){};
};

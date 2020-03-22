#pragma once
#include "../../Core/Memory/SmartPointers.h"

class GraphicsResource;
class String;

class GraphicsResourceType {
public:
    typedef void(*DeleteFn)(GraphicsResource*);
private:
    GraphicsResource* resource = nullptr;

    
    DeleteFn deleteResource;

public:

    GraphicsResourceType() {
        resource = nullptr;
        deleteResource = nullptr;
    }

    GraphicsResourceType(GraphicsResource* resource, DeleteFn deleteFunc)
    {
        resource = resource;
        deleteResource = deleteFunc;
    }

    GraphicsResourceType(const GraphicsResourceType& other)
    {
        resource = other.resource;
        deleteResource = other.deleteResource;
    }

    virtual ~GraphicsResourceType()
    {
        if (resource)
        {
            (*deleteResource)(resource);
        }
    }

    bool operator==(const GraphicsResourceType& otherType) const
    {
        return resource == otherType.resource;
    }

    bool operator==(const GraphicsResourceType* otherType) const
    {
        return resource == otherType->resource;
    }

    bool isChildOf(const GraphicsResourceType* otherType) const
    {
        return this == otherType || verifyParent(otherType);
    }

    template<typename CheckType>
    constexpr bool isChildOf() const
    {
        return this == CheckType::staticType() || verifyParent(CheckType::staticType());
    }

    virtual bool verifyParent(const GraphicsResourceType* otherType) const
    {
        return false;
    }
};

template<typename Parent,typename Child>
class GraphicsResourceTypeSpecialized : public GraphicsResourceType
{
protected:
    typedef typename Child ThisType;
    typedef typename Parent ParentType;


    constexpr bool verifyParent(const GraphicsResourceType* otherType) const
    {
        return ThisType::staticType() != ParentType::staticType() && ParentType::staticType()->isChildOf(otherType);
    }
public:

    GraphicsResourceTypeSpecialized(GraphicsResource* resource, GraphicsResourceType::DeleteFn deleteFunc):GraphicsResourceType(resource,deleteFunc)
    {}

    GraphicsResourceTypeSpecialized(const GraphicsResourceTypeSpecialized& other) :GraphicsResourceType(other)
    {}
};

#ifndef DECLARE_GRAPHICS_RESOURCE
#define DECLARE_GRAPHICS_RESOURCE(NewTypeName,NewTypeTemplates,BaseTypeName,BaseTypeTemplates)\
private: \
    typedef NewTypeName##NewTypeTemplates NewType; \
    typedef BaseTypeName##BaseTypeTemplates BaseType; \
protected:\
    typedef GraphicsResourceTypeSpecialized<typename BaseType,typename NewType> NewTypeName##_Type;\
    static SharedPtr<typename NewTypeName##_Type> STATIC_TYPE;\
public:\
    virtual const GraphicsResourceType* getType() const;\
    static const GraphicsResourceType* staticType();\
private:\
    \
    static void delFn(GraphicsResource* resource); \
    \
protected:\
    \
    template <typename CheckType, typename BaseType>\
    struct TypeOfBase {\
        \
        static bool isTypeOfBase() {\
            return BaseType::template isTypeOf<CheckType>();\
        }\
    };\
    \
    template <typename CheckType>\
    struct TypeOfBase<CheckType, GraphicsResource> {\
        \
        static bool isTypeOfBase() {\
            return false;\
        }\
    };\
public:\
    \
    template <typename CheckType>\
    static bool isTypeOf(){\
        return TypeOfBase<CheckType, GraphicsResource>::isTypeOfBase();\
    }\
    \
    template <>\
    static bool isTypeOf<GraphicsResource>() {\
        return true;\
    }
#endif // DECLARE_GRAPHICS_RESOURCE


#ifndef DEFINE_GRAPHICS_RESOURCE
#define DEFINE_GRAPHICS_RESOURCE(NewTypeName)\
    SharedPtr<NewTypeName::##NewTypeName##_Type> NewTypeName::STATIC_TYPE=SharedPtr<NewTypeName::##NewTypeName##_Type>(new NewTypeName::##NewTypeName##_Type(new NewTypeName(), &NewTypeName::delFn));\
    \
    void NewTypeName::delFn(GraphicsResource* resource) \
    { \
        delete resource; \
    } \
    const GraphicsResourceType* NewTypeName::getType() const { return STATIC_TYPE.get();}\
    const GraphicsResourceType* NewTypeName::staticType() { return STATIC_TYPE.get();}
#endif

#ifndef DEFINE_TEMPLATED_GRAPHICS_RESOURCE
#define DEFINE_TEMPLATED_GRAPHICS_RESOURCE(NewTypeName,NewTypeTemplates,TemplatesDefine)\
    template ##NewTypeTemplates## \
    SharedPtr<typename NewTypeName##TemplatesDefine##::##NewTypeName##_Type> NewTypeName##TemplatesDefine## \
        ::STATIC_TYPE=SharedPtr<typename NewTypeName##TemplatesDefine##::##NewTypeName##_Type>( \
        new typename NewTypeName##TemplatesDefine##::##NewTypeName##_Type(new typename NewTypeName##TemplatesDefine##::NewType(),\
        &NewTypeName##TemplatesDefine##::delFn));\
    \
    template ##NewTypeTemplates## \
    void NewTypeName##TemplatesDefine##::delFn(GraphicsResource* resource) \
    { \
        delete resource; \
    } \
    template ##NewTypeTemplates## \
    const GraphicsResourceType* NewTypeName##TemplatesDefine##::getType() const { return STATIC_TYPE.get();}\
    template ##NewTypeTemplates## \
    const GraphicsResourceType* NewTypeName##TemplatesDefine##::staticType() { return STATIC_TYPE.get();}
#endif // DEFINE_GRAPHICS_RESOURCE

class GraphicsResource
{
    DECLARE_GRAPHICS_RESOURCE(GraphicsResource,,GraphicsResource,)
    
public:
    GraphicsResource() = default;
    GraphicsResource(const GraphicsResource& otherResource) = delete;
    GraphicsResource(GraphicsResource&& otherResource) = delete;
    virtual ~GraphicsResource() = default;

    virtual void init() {};
    virtual void reinitResources() {};
    virtual void release() {};
    virtual String getResourceName() const;
};

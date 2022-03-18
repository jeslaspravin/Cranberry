/*!
 * \file CustomProperty.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "Property/Property.h"
#include "Types/PropertyTypes.h"
#include "Types/Containers/ReferenceCountPtr.h"

class CustomProperty;

// Helper class that can help with data access of advanced data types that do not have reflection informations
class PropertyDataRetriever
{
public:
    const CustomProperty* ownerProperty;
public:
    PropertyDataRetriever() = default;
    virtual ~PropertyDataRetriever() = default;

    FORCE_INLINE PropertyDataRetriever* setOwnerProperty(const CustomProperty* inOwnerProp)
    {
        ownerProperty = inOwnerProp;
        return this;
    }

    template <typename CastAs>
    FORCE_INLINE CastAs* thisAs()
    {
        return static_cast<CastAs*>(this);
    }
};

class REFLECTIONRUNTIME_EXPORT CustomProperty : public TypedProperty
{
public:
    const PropertyDataRetriever* dataRetriever;
public:
    CustomProperty(const StringID& propName, EPropertyType propType, const ReflectTypeInfo* propTypeInfo);
    ~CustomProperty();

    template <typename ConstructType, typename... CTorArgs>
    FORCE_INLINE ConstructType* constructDataRetriever(CTorArgs&&... args)
    {
        ConstructType* retriever = static_cast<ConstructType*>((new ConstructType(std::forward<CTorArgs>(args)...))->setOwnerProperty(this));
        dataRetriever = retriever;
        return retriever;
    }

    template <typename CastAs>
    FORCE_INLINE CastAs* thisAs()
    {
        return static_cast<CastAs*>(this);
    }
};


// std::pair property

class REFLECTIONRUNTIME_EXPORT PairDataRetriever : public PropertyDataRetriever
{
public:
    PairDataRetriever() = default;

    // Data retrieving functions
    virtual void* first(void* pairPtr) const = 0;
    virtual void* second(void* pairPtr) const = 0;
    virtual const void* first(const void* pairPtr) const = 0;
    virtual const void* second(const void* pairPtr) const = 0;
};

template <typename FirstType, typename SecondType>
class PairDataRetrieverImpl : public PairDataRetriever
{
public:
    using FirstFieldWrapper = MemberFieldWrapperImpl<std::pair<FirstType, SecondType>, FirstType>;
    using SecondFieldWrapper = MemberFieldWrapperImpl<std::pair<FirstType, SecondType>, SecondType>;
private:
    FirstFieldWrapper firstField = FirstFieldWrapper(&std::pair<FirstType, SecondType>::first);
    SecondFieldWrapper secondField = SecondFieldWrapper(&std::pair<FirstType, SecondType>::second);

public:
    void* first(void* pairPtr) const override
    {
        return firstField.get(pairPtr);
    }
    void* second(void* pairPtr) const override
    {
        return secondField.get(pairPtr);
    }
    const void* first(const void* pairPtr) const override
    {
        return firstField.get(pairPtr);
    }
    const void* second(const void* pairPtr) const override
    {
        return secondField.get(pairPtr);
    }
};

class REFLECTIONRUNTIME_EXPORT PairProperty final : public CustomProperty
{
public:
    const BaseProperty* keyProp;
    const BaseProperty* valueProp;

public:
    PairProperty(const StringID& propName, const ReflectTypeInfo* propTypeInfo)
        : CustomProperty(propName, EPropertyType::PairType, propTypeInfo)
    {}

    FORCE_INLINE PairProperty* setFirstProperty(const BaseProperty* firstProperty)
    {
        keyProp = firstProperty;
        return this;
    }

    FORCE_INLINE PairProperty* setSecondProperty(const BaseProperty* secondProperty)
    {
        valueProp = secondProperty;
        return this;
    }
};

// Iterate able data retriever
class REFLECTIONRUNTIME_EXPORT IteratorElementWrapper : public RefCountable
{
public:
    virtual void* getElement() const = 0;
    // Gets const element in iterators like set or unordered set where it is not possible to edit the element in the container
    virtual const void* getConstElement() const = 0;
    // ++Iterator
    virtual void iterateFwd() = 0;
    // --Iterator
    virtual void iterateBwd() = 0;
    // Iterator != end
    virtual bool isValid() const = 0;
};
using IteratorElementWrapperRef = ReferenceCountPtr<IteratorElementWrapper>;

class REFLECTIONRUNTIME_EXPORT IterateableDataRetriever : public PropertyDataRetriever
{
public:
    // Creates a Iterator for the provided object. From the beginning
    // Do not take hold of the returned reference for persistent use as underlying iterator lifetime is not managed
    virtual IteratorElementWrapperRef createIterator(void* object) const = 0;
};

// std::map, std::unordered_map
class REFLECTIONRUNTIME_EXPORT MapIteratorWrapper : public IteratorElementWrapper
{
public:
    virtual const void* key() const = 0;
    virtual void* value() const = 0;
};

template <typename MapType, typename IteratorType = MapType::iterator>
class MapIteratorWrapperImpl : public MapIteratorWrapper
{
private:
    MapType* containerPtr;
public:
    IteratorType itr;
public:
    MapIteratorWrapperImpl(MapType* inMapPtr)
        : containerPtr(inMapPtr)
        , itr(inMapPtr->begin())
    {}

    /* IteratorElementWrapper overrides */
    void* getElement() const override
    {
        return &(*itr);
    }
    const void* getConstElement() const override
    {
        return &(*itr);
    }
    void iterateFwd() override
    {
        ++itr;
    }
    void iterateBwd() override
    {
        --itr;
    }
    bool isValid() const override
    {
        return itr != containerPtr->end();
    }
    /* MapIteratorWrapper overrides */

    const void* key() const override
    {
        return &(itr->first);
    }
    void* value() const override
    {
        return &(itr->second);
    }
    /* Override ends */
};

template <typename MapType>
class MapDataRetrieverImpl : public IterateableDataRetriever
{
public:
    IteratorElementWrapperRef createIterator(void* object) const override
    {
        return IteratorElementWrapperRef(new MapIteratorWrapperImpl<MapType>((MapType*)(object)));
    }
};

class REFLECTIONRUNTIME_EXPORT MapProperty final : public CustomProperty
{
public:
    const BaseProperty* keyProp;
    const BaseProperty* valueProp;
    // Will be std::pair<const KeyType, ValueType>
    // Element prop will be nullptr unless the type is reflected in function parameters or reflected field
    const BaseProperty* elementProp;
public:
    MapProperty(const StringID& propName, const ReflectTypeInfo* propTypeInfo)
        : CustomProperty(propName, EPropertyType::MapType, propTypeInfo)
    {}

    FORCE_INLINE MapProperty* setKeyProperty(const BaseProperty* keyProperty)
    {
        keyProp = keyProperty;
        return this;
    }

    FORCE_INLINE MapProperty* setValueProperty(const BaseProperty* valueProperty)
    {
        valueProp = valueProperty;
        return this;
    }

    FORCE_INLINE MapProperty* setElementProperty(const BaseProperty* elementProperty)
    {
        elementProp = elementProperty;
        return this;
    }
};

// Single element iterators like std::set, std::unordered_set, std::vector
class REFLECTIONRUNTIME_EXPORT IndexableIteratorWrapper : public IteratorElementWrapper
{
public:
    virtual void* operator[](int64 diff) const = 0;
};

template <typename ContainerType, typename IteratorType = ContainerType::iterator>
class ContainerIteratorWrapperImpl : public IteratorElementWrapper
{
private:
    ContainerType* containerPtr;
public:
    IteratorType itr;
public:
    ContainerIteratorWrapperImpl(ContainerType* inMapPtr)
        : containerPtr(inMapPtr)
        , itr(inMapPtr->begin())
    {}

    /* IteratorElementWrapper overrides */
    void* getElement() const override
    {
        if CONST_EXPR (std::is_const_v<UnderlyingTypeWithConst<ContainerType::iterator::reference>>)
        {
            return nullptr;
        }
        else
        {
            return &(*itr);
        }
    }
    const void* getConstElement() const override
    {
        return &(*itr);
    }

    void iterateFwd() override
    {
        ++itr;
    }
    void iterateBwd() override
    {
        --itr;
    }
    bool isValid() const override
    {
        return itr != containerPtr->end();
    }
    /* Override ends */
};

template <typename ContainerType, typename IteratorType = ContainerType::iterator>
class IndexableContainerIteratorWrapperImpl : public IndexableIteratorWrapper
{
private:
    ContainerType* containerPtr;
public:
    IteratorType itr;

public:
    IndexableContainerIteratorWrapperImpl(ContainerType* inContainerPtr)
        : containerPtr(inContainerPtr)
        , itr(inContainerPtr->begin())
    {}

    /* IteratorElementWrapper overrides */
    void* getElement() const override
    {
        return &(*itr);
    }
    const void* getConstElement() const override
    {
        return &(*itr);
    }

    void iterateFwd() override
    {
        ++itr;
    }
    void iterateBwd() override
    {
        --itr;
    }
    bool isValid() const override
    {
        return itr != containerPtr->end();
    }
    /* IndexableIteratorWrapper overrides */
    void* operator[](int64 diff) const override
    {
        return &(itr[diff]);
    }
    /* Override ends */
};

// If indexable then we create indexable iterator
template <typename ContainerType>
class ContainerRetrieverImpl : public IterateableDataRetriever
{
private:
    static IteratorElementWrapperRef createIteratorImpl(void* object)
        requires Indexable<ContainerType>
    {
        return IteratorElementWrapperRef(new IndexableContainerIteratorWrapperImpl<ContainerType>((ContainerType*)(object)));
    }
    static IteratorElementWrapperRef createIteratorImpl(void* object) 
        requires (!Indexable<ContainerType>)
    {
        return IteratorElementWrapperRef(new ContainerIteratorWrapperImpl<ContainerType>((ContainerType*)(object)));
    }
public:
    IteratorElementWrapperRef createIterator(void* object) const override
    {
        return createIteratorImpl(object);
    }
};

class REFLECTIONRUNTIME_EXPORT ContainerProperty : public CustomProperty
{
public:
    const BaseProperty* elementProp;
public:
    ContainerProperty(const StringID& propName, EPropertyType propType, const ReflectTypeInfo* propTypeInfo)
        : CustomProperty(propName, propType, propTypeInfo)
    {}

    FORCE_INLINE ContainerProperty* setElementProperty(const BaseProperty* elementProperty)
    {
        elementProp = elementProperty;
        return this;
    }
};
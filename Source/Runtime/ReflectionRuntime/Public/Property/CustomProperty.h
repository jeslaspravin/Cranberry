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
#include "Types/Containers/ReferenceCountPtr.h"
#include "Types/PropertyTypes.h"

class CustomProperty;

// Helper class that can help with data access of advanced data types that do not have reflection
// informations
class PropertyDataRetriever
{
public:
    const CustomProperty *ownerProperty;

public:
    PropertyDataRetriever() = default;
    virtual ~PropertyDataRetriever() = default;

    FORCE_INLINE PropertyDataRetriever *setOwnerProperty(const CustomProperty *inOwnerProp)
    {
        ownerProperty = inOwnerProp;
        return this;
    }

    template <typename CastAs>
    FORCE_INLINE CastAs *thisAs()
    {
        return static_cast<CastAs *>(this);
    }
};

class REFLECTIONRUNTIME_EXPORT CustomProperty : public TypedProperty
{
public:
    const PropertyDataRetriever *dataRetriever;

public:
    CustomProperty(const StringID &propNameID, const String &propName, EPropertyType propType, const ReflectTypeInfo *propTypeInfo);
    ~CustomProperty();

    template <typename ConstructType, typename... CTorArgs>
    FORCE_INLINE ConstructType *constructDataRetriever(CTorArgs &&...args)
    {
        ConstructType *retriever = static_cast<ConstructType *>((new ConstructType(std::forward<CTorArgs>(args)...))->setOwnerProperty(this));
        dataRetriever = retriever;
        return retriever;
    }

    template <typename CastAs>
    FORCE_INLINE CastAs *thisAs()
    {
        return static_cast<CastAs *>(this);
    }
};

// std::pair property

class REFLECTIONRUNTIME_EXPORT PairDataRetriever : public PropertyDataRetriever
{
public:
    PairDataRetriever() = default;

    // Data retrieving functions
    virtual void *first(void *pairPtr) const = 0;
    virtual void *second(void *pairPtr) const = 0;
    virtual const void *first(const void *pairPtr) const = 0;
    virtual const void *second(const void *pairPtr) const = 0;
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
    void *first(void *pairPtr) const override { return firstField.get(pairPtr); }
    void *second(void *pairPtr) const override { return secondField.get(pairPtr); }
    const void *first(const void *pairPtr) const override { return firstField.get(pairPtr); }
    const void *second(const void *pairPtr) const override { return secondField.get(pairPtr); }
};

class REFLECTIONRUNTIME_EXPORT PairProperty final : public CustomProperty
{
public:
    const BaseProperty *keyProp;
    const BaseProperty *valueProp;

public:
    PairProperty(const StringID &propNameID, const String &propName, const ReflectTypeInfo *propTypeInfo)
        : CustomProperty(propNameID, propName, EPropertyType::PairType, propTypeInfo)
        , keyProp(nullptr)
        , valueProp(nullptr)
    {}

    FORCE_INLINE PairProperty *setFirstProperty(const BaseProperty *firstProperty)
    {
        keyProp = firstProperty;
        return this;
    }

    FORCE_INLINE PairProperty *setSecondProperty(const BaseProperty *secondProperty)
    {
        valueProp = secondProperty;
        return this;
    }
};

// Iterate able data retriever
class REFLECTIONRUNTIME_EXPORT IteratorElementWrapper : public RefCountable
{
public:
    virtual void *getElement() const = 0;
    // Gets const element in iterators like set or unordered set where it is not possible to edit the
    // element in the container
    virtual const void *getConstElement() const = 0;
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
    // Do not take hold of the returned reference for persistent use as underlying iterator lifetime is
    // not managed
    virtual IteratorElementWrapperRef createIterator(void *object) const = 0;
    // Add and remove operations for editing, value editing can be done on iterator itself
    virtual bool add(void *object, const void *data, bool bTryForced = false) const = 0;
    virtual bool remove(void *object, const void *data) const = 0;
    // For indexable iterators only
    virtual bool removeAt(void *object, SizeT idx) const = 0;
    virtual SizeT size(const void *object) const = 0;
    virtual void clear(void *object) const = 0;

    // Helper functions to helper with editing
    // Copied data useful for cases like map or set as they are restricted by const key
    virtual void copyTo(const void *data, void *toData) const = 0;
    virtual void contruct(void *data) const = 0;
    // Equals only const part which will be key_type/value_type based on container
    virtual bool equals(const void *lhs, const void *rhs) const = 0;
};

// std::map, std::unordered_map
class REFLECTIONRUNTIME_EXPORT MapIteratorWrapper : public IteratorElementWrapper
{
public:
    virtual const void *key() const = 0;
    virtual void *value() const = 0;
};
using MapIteratorWrapperRef = ReferenceCountPtr<MapIteratorWrapper>;

template <typename MapType, typename IteratorType = MapType::iterator>
class MapIteratorWrapperImpl : public MapIteratorWrapper
{
private:
    MapType *containerPtr;

public:
    IteratorType itr;

public:
    MapIteratorWrapperImpl(MapType *inMapPtr)
        : containerPtr(inMapPtr)
        , itr(inMapPtr->begin())
    {}

    /* IteratorElementWrapper overrides */
    void *getElement() const override { return &(*itr); }
    const void *getConstElement() const override { return &(*itr); }
    void iterateFwd() override { ++itr; }
    void iterateBwd() override { --itr; }
    bool isValid() const override { return itr != containerPtr->end(); }
    /* MapIteratorWrapper overrides */

    const void *key() const override { return &(itr->first); }
    void *value() const override { return &(itr->second); }
    /* Override ends */
};

template <typename MapType>
class MapDataRetrieverImpl : public IterateableDataRetriever
{
public:
    IteratorElementWrapperRef createIterator(void *object) const override
    {
        return IteratorElementWrapperRef(new MapIteratorWrapperImpl<MapType>((MapType *)(object)));
    }

    bool add(void *object, const void *data, bool bTryForced = false) const override
    {
        MapType *container = reinterpret_cast<MapType *>(object);
        const MapType::value_type *pairVal = reinterpret_cast<const MapType::value_type *>(data);
        if (bTryForced)
        {
            (*container)[pairVal->first] = pairVal->second;
            return true;
        }
        else
        {
            auto result = container->insert(*pairVal);
            return result.second;
        }
    }

    bool remove(void *object, const void *data) const override
    {
        MapType *container = reinterpret_cast<MapType *>(object);
        auto itr = container->find(*reinterpret_cast<const MapType::key_type *>(data));
        if (itr != container->end())
        {
            container->erase(itr);
            return true;
        }
        return false;
    }

    bool removeAt(void *object, SizeT idx) const override { return false; }

    SizeT size(const void *object) const override { return reinterpret_cast<const MapType *>(object)->size(); }
    void clear(void *object) const override { reinterpret_cast<MapType *>(object)->clear(); }

    void copyTo(const void *data, void *toData) const override
    {
        new (toData) MapType::value_type(*reinterpret_cast<const MapType::value_type *>(data));
    }
    void contruct(void *data) const override { new (data) MapType::value_type(); }
    bool equals(const void *lhs, const void *rhs) const
    {
        return (*reinterpret_cast<const MapType::key_type *>(lhs)) == (*reinterpret_cast<const MapType::key_type *>(rhs));
    }
};

class REFLECTIONRUNTIME_EXPORT MapProperty final : public CustomProperty
{
public:
    const BaseProperty *keyProp;
    const BaseProperty *valueProp;
    // Will be std::pair<const KeyType, ValueType>
    // Element prop will be nullptr unless the type is reflected in function parameters or reflected
    // field
    const BaseProperty *elementProp;
    uint32 pairSize;
    uint32 pairAlignment;
    uint32 secondOffset;

public:
    MapProperty(const StringID &propNameID, const String &propName, const ReflectTypeInfo *propTypeInfo)
        : CustomProperty(propNameID, propName, EPropertyType::MapType, propTypeInfo)
        , keyProp(nullptr)
        , valueProp(nullptr)
        , elementProp(nullptr)
        , pairSize(0)
        , pairAlignment(0)
        , secondOffset(0)
    {}

    FORCE_INLINE MapProperty *setKeyValueProperties(const BaseProperty *keyProperty, const BaseProperty *valueProperty)
    {
        keyProp = keyProperty;
        valueProp = valueProperty;

        const TypedProperty *kProp = static_cast<const TypedProperty *>(keyProp);
        const TypedProperty *vProp = static_cast<const TypedProperty *>(valueProp);
        pairAlignment = Math::max(kProp->typeInfo->alignment, vProp->typeInfo->alignment);
        pairSize = secondOffset = Math::alignByUnsafe(kProp->typeInfo->size, vProp->typeInfo->alignment);
        pairSize = Math::alignByUnsafe(pairSize + vProp->typeInfo->size, pairAlignment);

        return this;
    }

    FORCE_INLINE MapProperty *setElementProperty(const BaseProperty *elementProperty)
    {
        elementProp = elementProperty;
        return this;
    }
};

// Single element iterators like std::set, std::unordered_set, std::vector
class REFLECTIONRUNTIME_EXPORT IndexableIteratorWrapper : public IteratorElementWrapper
{
public:
    virtual void *operator[](int64 diff) const = 0;
};

template <typename ContainerType, typename IteratorType = ContainerType::iterator>
class ContainerIteratorWrapperImpl : public IteratorElementWrapper
{
private:
    ContainerType *containerPtr;

public:
    IteratorType itr;

public:
    ContainerIteratorWrapperImpl(ContainerType *inMapPtr)
        : containerPtr(inMapPtr)
        , itr(inMapPtr->begin())
    {}

    /* IteratorElementWrapper overrides */
    void *getElement() const override
    {
        if CONST_EXPR (std::is_const_v<std::remove_reference_t<ContainerType::iterator::reference>>)
        {
            return nullptr;
        }
        else
        {
            return &(*itr);
        }
    }
    const void *getConstElement() const override { return &(*itr); }

    void iterateFwd() override { ++itr; }
    void iterateBwd() override { --itr; }
    bool isValid() const override { return itr != containerPtr->end(); }
    /* Override ends */
};

template <typename ContainerType, typename IteratorType = ContainerType::iterator>
class IndexableContainerIteratorWrapperImpl : public IndexableIteratorWrapper
{
private:
    ContainerType *containerPtr;

public:
    IteratorType itr;

public:
    IndexableContainerIteratorWrapperImpl(ContainerType *inContainerPtr)
        : containerPtr(inContainerPtr)
        , itr(inContainerPtr->begin())
    {}

    /* IteratorElementWrapper overrides */
    void *getElement() const override { return &(*itr); }
    const void *getConstElement() const override { return &(*itr); }

    void iterateFwd() override { ++itr; }
    void iterateBwd() override { --itr; }
    bool isValid() const override { return itr != containerPtr->end(); }
    /* IndexableIteratorWrapper overrides */
    void *operator[](int64 diff) const override { return &(itr[diff]); }
    /* Override ends */
};

template <typename ContainerType>
class ContainerRetrieverImpl;

// If indexable then we create indexable iterator
template <Indexable ContainerType>
class ContainerRetrieverImpl<ContainerType> : public IterateableDataRetriever
{
public:
    IteratorElementWrapperRef createIterator(void *object) const override
    {
        return IteratorElementWrapperRef(new IndexableContainerIteratorWrapperImpl<ContainerType>((ContainerType *)(object)));
    }

    bool add(void *object, const void *data, bool bTryForced = false) const override
    {
        ContainerType *container = reinterpret_cast<ContainerType *>(object);
        container->emplace_back(*reinterpret_cast<const ContainerType::value_type *>(data));
        return true;
    }

    bool remove(void *object, const void *data) const override
    {
        ContainerType *container = reinterpret_cast<ContainerType *>(object);
        if CONST_EXPR (std::equality_comparable<ContainerType::value_type>)
        {
            auto itr = std::find(container->begin(), container->end(), *reinterpret_cast<const ContainerType::value_type *>(data));
            if (itr != container->end())
            {
                container->erase(itr);
                return true;
            }
        }
        alertIf(std::equality_comparable<ContainerType::value_type>, "LOGICAL ERROR: Type does not have equality check!");
        return false;
    }

    bool removeAt(void *object, SizeT idx) const override
    {
        ContainerType *container = reinterpret_cast<ContainerType *>(object);
        debugAssert(container && container->size() > idx);
        container->erase(container->begin() + idx);
        return true;
    }

    SizeT size(const void *object) const override { return reinterpret_cast<const ContainerType *>(object)->size(); }
    void clear(void *object) const override { reinterpret_cast<ContainerType *>(object)->clear(); }

    void copyTo(const void *data, void *toData) const override
    {
        new (toData) ContainerType::value_type(*reinterpret_cast<const ContainerType::value_type *>(data));
    }
    void contruct(void *data) const override { new (data) ContainerType::value_type(); }
    bool equals(const void *lhs, const void *rhs) const
    {
        if CONST_EXPR (std::equality_comparable<ContainerType::value_type>)
        {
            return (*reinterpret_cast<const ContainerType::value_type *>(lhs)) == (*reinterpret_cast<const ContainerType::value_type *>(rhs));
        }
        alertIf(std::equality_comparable<ContainerType::value_type>, "LOGICAL ERROR: Type does not have equality check!");
        return false;
    }
};

// For set/unrodered_set
template <typename ContainerType>
requires(!Indexable<ContainerType>) class ContainerRetrieverImpl<ContainerType> : public IterateableDataRetriever
{
public:
    IteratorElementWrapperRef createIterator(void *object) const override
    {
        return IteratorElementWrapperRef(new ContainerIteratorWrapperImpl<ContainerType>((ContainerType *)(object)));
    }

    bool add(void *object, const void *data, bool bTryForced = false) const override
    {
        ContainerType *container = reinterpret_cast<ContainerType *>(object);
        auto result = container->insert(*reinterpret_cast<const ContainerType::value_type *>(data));
        return result.second;
    }

    bool remove(void *object, const void *data) const override
    {
        ContainerType *container = reinterpret_cast<ContainerType *>(object);
        auto itr = container->find(*reinterpret_cast<const ContainerType::value_type *>(data));
        if (itr != container->end())
        {
            container->erase(itr);
            return true;
        }
        return false;
    }

    bool removeAt(void *object, SizeT idx) const override { return false; }

    SizeT size(const void *object) const override { return reinterpret_cast<const ContainerType *>(object)->size(); }
    void clear(void *object) const override { reinterpret_cast<ContainerType *>(object)->clear(); }

    void copyTo(const void *data, void *toData) const override
    {
        new (toData) ContainerType::value_type(*reinterpret_cast<const ContainerType::value_type *>(data));
    }
    void contruct(void *data) const override { new (data) ContainerType::value_type(); }
    bool equals(const void *lhs, const void *rhs) const
    {
        return (*reinterpret_cast<const ContainerType::value_type *>(lhs)) == (*reinterpret_cast<const ContainerType::value_type *>(rhs));
    }
};

class REFLECTIONRUNTIME_EXPORT ContainerProperty : public CustomProperty
{
public:
    const BaseProperty *elementProp;

public:
    ContainerProperty(const StringID &propNameID, const String &propName, EPropertyType propType, const ReflectTypeInfo *propTypeInfo)
        : CustomProperty(propNameID, propName, propType, propTypeInfo)
        , elementProp(nullptr)
    {}

    FORCE_INLINE ContainerProperty *setElementProperty(const BaseProperty *elementProperty)
    {
        elementProp = elementProperty;
        return this;
    }
};
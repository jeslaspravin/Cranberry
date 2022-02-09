/*!
 * \file PropertyTypes.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once
#include "Types/TypesInfo.h"
#include "Reflections/Fields.h"
#include "Logger/Logger.h"


class BaseFieldWrapper
{
private:
    const ReflectTypeInfo* propertyTypeInfo;
protected:
    virtual const void* propertyAccessor() const = 0;
public:
    BaseFieldWrapper(const ReflectTypeInfo* propertyType)
        : propertyTypeInfo(propertyType)
    {}

    virtual ~BaseFieldWrapper() = default;

    const ReflectTypeInfo* getPropertyTypeInfo() const
    {
        return propertyTypeInfo;
    }

    // Including all CV-Ref qualifiers
    template <typename CheckType>
    FORCE_INLINE bool isSameType() const
    {
        return propertyTypeInfo == typeInfoFrom<CheckType>();
    }

    // Non CV-Ref qualifiers
    // If const int& is full type this method checks if CheckType is same as int
    template <typename CheckType>
    FORCE_INLINE bool isSameBasicType() const
    {
        return propertyTypeInfo->typeID == std::type_index(typeid(CheckType));
    }
};

template <typename PropertyType>
union FieldValuePtr
{
    PropertyType* vPtr;
    const PropertyType* constVPtr;
    void* ptr;

    FieldValuePtr(PropertyType* nonConstCtor)
        : vPtr(nonConstCtor)
    {}

    FieldValuePtr(const PropertyType* constCtor)
        : constVPtr(constCtor)
    {}

    FieldValuePtr()
        : ptr(nullptr)
    {}

    operator bool() const
    {
        return (ptr != nullptr);
    }
};


class MemberFieldWrapper : public BaseFieldWrapper
{
private:
    const ReflectTypeInfo* memberOfType;
public:
    MemberFieldWrapper(const ReflectTypeInfo* outerClassType, const ReflectTypeInfo* propertyType)
        : BaseFieldWrapper(propertyType)
        , memberOfType(outerClassType)
    {}

    FORCE_INLINE const ReflectTypeInfo* getMemberOfType() const
    {
        return memberOfType;
    }

    template <typename CheckType>
    FORCE_INLINE bool isMemberOfSameType() const
    {
        return memberOfType == typeInfoFrom<CheckType>();
    }
    
    virtual void* get(void* object) const = 0;
    virtual const void* get(const void* object) const = 0;

    // Will return pointer to value else null
    template <typename AsType, typename ObjectType>
    FieldValuePtr<AsType> getAsType(ObjectType&& object) const
    {
        // TODO : Change the constness logic for pointer types
        static_assert(std::disjunction_v<std::is_pointer<ObjectType>, std::is_reference<ObjectType>>, "Must be a pointer or a reference type");

        // Const-ness can be determined in non const type
        const ReflectTypeInfo* objectTypeInfo = typeInfoFrom<std::remove_pointer_t<ObjectType>>();
        if (isMemberOfSameType<CleanType<ObjectType>>() && isSameType<AsType>())
        {
            // If constant then we use const MemberDataPointer or if object is const
            if (BIT_SET(getPropertyTypeInfo()->qualifiers, EReflectTypeQualifiers::Constant) 
                || BIT_SET(objectTypeInfo->qualifiers, EReflectTypeQualifiers::Constant))
            {
                const AsType* retVal = nullptr;
                const ClassMemberField<true, CleanType<ObjectType>, AsType>* memberFieldPtr
                    = (const ClassMemberField<true, CleanType<ObjectType>, AsType>*)(propertyAccessor());

                fatalAssert(memberFieldPtr, "%s() : Invalid member pointer", __func__);
                retVal = &memberFieldPtr->get(std::forward<ObjectType>(object));
                return retVal;
            }
            else
            {
                AsType* retVal = nullptr;
                const ClassMemberField<false, CleanType<ObjectType>, AsType>* memberFieldPtr
                    = (const ClassMemberField<false, CleanType<ObjectType>, AsType>*)(propertyAccessor());

                fatalAssert(memberFieldPtr, "%s() : Invalid member pointer", __func__);
                retVal = &memberFieldPtr->get(std::forward<ObjectType>(object));
                return retVal;
            }
        }
        return {};
    }

    template <typename FromType, typename ObjectType>
    bool setFromType(FromType&& value, ObjectType&& object) const
    {
        using MemberType = std::remove_cvref_t<FromType>;

        const ReflectTypeInfo* objectTypeInfo = typeInfoFrom<std::remove_pointer_t<ObjectType>>();
        if (isMemberOfSameType<CleanType<ObjectType>>() && isSameType<MemberType>())
        {
            // If constant then we use const MemberDataPointer
            if (BIT_SET(getPropertyTypeInfo()->qualifiers, EReflectTypeQualifiers::Constant)
                || BIT_SET(objectTypeInfo->qualifiers, EReflectTypeQualifiers::Constant))
            {
                LOG_ERROR("MemberDataProperty", "%s() : Cannot set constant value", __func__);
            }
            else
            {
                const ClassMemberField<false, CleanType<ObjectType>, MemberType>* memberFieldPtr
                    = (const ClassMemberField<false, CleanType<ObjectType>, MemberType>*)(propertyAccessor());

                fatalAssert(memberFieldPtr, "%s() : Invalid member pointer", __func__);
                memberFieldPtr->set(std::forward<ObjectType>(object), std::forward<FromType>(value));
                return true;
            }
        }
        return false;
    }
};

class GlobalFieldWrapper : public BaseFieldWrapper
{
public:
    GlobalFieldWrapper(const ReflectTypeInfo* propertyType)
        : BaseFieldWrapper(propertyType)
    {}

    virtual FieldValuePtr<void> get() const = 0;

    // Will return pointer to value else null
    template <typename AsType>
    FieldValuePtr<AsType> getAsType()
    {
        if (isSameType<AsType>())
        {
            // If constant then we use const MemberDataPointer
            if (BIT_SET(getPropertyTypeInfo()->qualifiers, EReflectTypeQualifiers::Constant))
            {
                const AsType* retVal = nullptr;
                const GlobalField<true, AsType>* memberFieldPtr
                    = (const GlobalField<true, AsType>*)(propertyAccessor());

                fatalAssert(memberFieldPtr, "%s() : Invalid Field pointer", __func__);
                retVal = &memberFieldPtr->get();
                return retVal;
            }
            else
            {
                AsType* retVal = nullptr;
                const GlobalField<false, AsType>* memberFieldPtr
                    = (const GlobalField<false, AsType>*)(propertyAccessor());

                fatalAssert(memberFieldPtr, "%s() : Invalid Field pointer", __func__);
                retVal = &memberFieldPtr->get();
                return retVal;
            }
        }
        return {};
    }

    template <typename FromType>
    bool setFromType(FromType&& value)
    {
        using MemberType = std::remove_cvref_t<FromType>;

        if (isSameType<MemberType>())
        {
            // If constant then we use const MemberDataPointer
            if (BIT_SET(getPropertyTypeInfo()->qualifiers, EReflectTypeQualifiers::Constant))
            {
                LOG_ERROR("MemberDataProperty", "%s() : Cannot set constant value", __func__);
            }
            else
            {
                const GlobalField<false, MemberType>* memberFieldPtr
                    = (const GlobalField<false, MemberType>*)(propertyAccessor());

                fatalAssert(memberFieldPtr, "%s() : Invalid member pointer", __func__);
                memberFieldPtr->set(std::forward<FromType>(value));
                return true;
            }
        }
        return false;
    }
};

// Templated Implementations

template <typename ObjectType, typename MemberType>
class MemberFieldWrapperImpl : public MemberFieldWrapper
{
private:
    // Member property must not be a reference
    static_assert(!std::is_reference_v<MemberType>);
    using MemberFieldType = ClassMemberField<std::is_const_v<CleanType<MemberType>>, ObjectType, MemberType>;

    MemberFieldType memberField;
public:
    MemberFieldWrapperImpl(MemberFieldType::MemberFieldPtr memberPtr)
        : MemberFieldWrapper(typeInfoFrom<ObjectType>(), typeInfoFrom<MemberType>())
        , memberField(memberPtr)
    {}

    /* BaseFieldWrapper overrides */
protected:
    const void* propertyAccessor() const override
    {
        return &memberField;
    }
    /* MemberFieldWrapper overrides */
public:
    void* get(void* object) const override
    {
        if CONST_EXPR(std::is_const_v<MemberType>)
        {
            LOG_ERROR("MemberFieldWrapperImpl", "%s() : Use const object function to retrieve const value", __func__);
            return nullptr;
        }
        else
        {
            ObjectType* outerObject = (ObjectType*)(object);
            return &memberField.get(outerObject);
        }
    }

    const void* get(const void* object) const override
    {
        const ObjectType* outerObject = (const ObjectType*)(object);
        return &memberField.get(outerObject);
    }
    /* Override ends */
};

template <typename MemberType>
class GlobalFieldWrapperImpl : public GlobalFieldWrapper
{
private:
    // Member property must not be a reference
    static_assert(!std::is_reference_v<MemberType>);
    using FieldType = GlobalField<std::is_const_v<CleanType<MemberType>>, MemberType>;

    FieldType field;
public:
    GlobalFieldWrapperImpl(FieldType::GlobalFieldPtr fieldPtr)
        : GlobalFieldWrapper(typeInfoFrom<MemberType>())
        , field(fieldPtr)
    {}

    /* BaseFieldWrapper overrides */
protected:
    const void* propertyAccessor() const override
    {
        return &field;
    }
    /* GlobalFieldWrapper overrides */
public:
    FieldValuePtr<void> get() const override
    {
        return &field.get();
    }

    /* Override ends */
};
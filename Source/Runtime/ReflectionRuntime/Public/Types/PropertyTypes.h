/*!
 * \file PropertyTypes.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once
#include "Logger/Logger.h"
#include "Reflections/Fields.h"
#include "Types/TypesInfo.h"

class BaseFieldWrapper
{
private:
    const ReflectTypeInfo *propertyTypeInfo;

protected:
    virtual const void *propertyAccessor() const = 0;

public:
    BaseFieldWrapper(const ReflectTypeInfo *propertyType)
        : propertyTypeInfo(propertyType)
    {}

    virtual ~BaseFieldWrapper() = default;

    const ReflectTypeInfo *getPropertyTypeInfo() const { return propertyTypeInfo; }

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
    PropertyType *vPtr;
    const PropertyType *constVPtr;
    void *ptr;

    FieldValuePtr(PropertyType *nonConstCtor)
        : vPtr(nonConstCtor)
    {}

    FieldValuePtr(const PropertyType *constCtor)
        : constVPtr(constCtor)
    {}

    FieldValuePtr()
        : ptr(nullptr)
    {}

    operator bool () const { return (ptr != nullptr); }
};

class MemberFieldWrapper : public BaseFieldWrapper
{
private:
    const ReflectTypeInfo *memberOfType;

public:
    MemberFieldWrapper(const ReflectTypeInfo *outerClassType, const ReflectTypeInfo *propertyType)
        : BaseFieldWrapper(propertyType)
        , memberOfType(outerClassType)
    {}

    FORCE_INLINE const ReflectTypeInfo *getMemberOfType() const { return memberOfType; }

    template <typename CheckType>
    FORCE_INLINE bool isMemberOfSameType() const
    {
        return memberOfType == typeInfoFrom<CheckType>();
    }

    virtual void *get(void *object) const = 0;
    virtual const void *get(const void *object) const = 0;
    virtual void setTypeless(void *value, void *object) const = 0;

    // Will return pointer to value else null
    template <typename AsType, typename ObjectType>
    FieldValuePtr<AsType> getAsType(ObjectType &&object) const
    {
        if (isMemberOfSameType<CleanType<ObjectType>>() && isSameType<AsType>())
        {
            return getAsTypeUnsafe<AsType>(std::forward<ObjectType>(object));
        }
        return {};
    }
    template <typename FromType, typename ObjectType>
    bool setFromType(FromType &&value, ObjectType &&object) const
    {
        using MemberType = std::remove_cvref_t<FromType>;

        if (isMemberOfSameType<CleanType<ObjectType>>() && isSameType<MemberType>())
        {
            return setFromTypeUnsafe<FromType>(std::forward<FromType>(value), std::forward<ObjectType>(object));
        }
        return false;
    }

    template <typename AsType, typename ObjectType>
    FieldValuePtr<AsType> getAsTypeUnsafe(ObjectType &&object) const
    {
        if CONST_EXPR (std::is_const_v<UnderlyingTypeWithConst<ObjectType>>)
        {
            const AsType *retVal = nullptr;
            const ClassMemberField<true, CleanType<ObjectType>, AsType> *memberFieldPtr
                = (const ClassMemberField<true, CleanType<ObjectType>, AsType> *)(propertyAccessor());

            debugAssert(memberFieldPtr);
            retVal = &memberFieldPtr->get(std::forward<ObjectType>(object));
            return retVal;
        }
        else
        {
            // If constant then we use const MemberDataPointer or if object is const
            if (BIT_SET(getPropertyTypeInfo()->qualifiers, EReflectTypeQualifiers::Constant))
            {
                const AsType *retVal = nullptr;
                const ClassMemberField<true, CleanType<ObjectType>, AsType> *memberFieldPtr
                    = (const ClassMemberField<true, CleanType<ObjectType>, AsType> *)(propertyAccessor());

                debugAssert(memberFieldPtr);
                retVal = &memberFieldPtr->get(std::forward<ObjectType>(object));
                return retVal;
            }
            else
            {
                AsType *retVal = nullptr;
                const ClassMemberField<false, CleanType<ObjectType>, AsType> *memberFieldPtr
                    = (const ClassMemberField<false, CleanType<ObjectType>, AsType> *)(propertyAccessor());

                debugAssert(memberFieldPtr);
                retVal = &memberFieldPtr->get(std::forward<ObjectType>(object));
                return retVal;
            }
        }
    }

    template <typename FromType, typename ObjectType>
    bool setFromTypeUnsafe(FromType &&value, ObjectType &&object) const
    {
        using MemberType = std::remove_cvref_t<FromType>;

        if CONST_EXPR (std::is_const_v<UnderlyingTypeWithConst<ObjectType>>)
        {
            LOG_ERROR("MemberDataProperty", "Cannot set constant value");
        }
        else
        {
            // If constant then we use const MemberDataPointer
            if (BIT_SET(getPropertyTypeInfo()->qualifiers, EReflectTypeQualifiers::Constant))
            {
                LOG_ERROR("MemberDataProperty", "Cannot set constant value");
            }
            else
            {
                const ClassMemberField<false, CleanType<ObjectType>, MemberType> *memberFieldPtr
                    = (const ClassMemberField<false, CleanType<ObjectType>, MemberType> *)(propertyAccessor());

                debugAssert(memberFieldPtr);
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
    GlobalFieldWrapper(const ReflectTypeInfo *propertyType)
        : BaseFieldWrapper(propertyType)
    {}

    virtual FieldValuePtr<void> get() const = 0;
    virtual void setTypeless(void *value) const = 0;

    // Will return pointer to value else null
    template <typename AsType>
    FieldValuePtr<AsType> getAsType() const
    {
        if (isSameType<AsType>())
        {
            return getAsTypeUnsafe<AsType>();
        }
        return {};
    }

    template <typename FromType>
    bool setFromType(FromType &&value) const
    {
        using MemberType = std::remove_cvref_t<FromType>;

        if (isSameType<MemberType>())
        {
            return setFromTypeUnsafe(std::forward<FromType>(value));
        }
        return false;
    }

    template <typename AsType>
    FieldValuePtr<AsType> getAsTypeUnsafe() const
    {
        // If constant then we use const MemberDataPointer
        if (BIT_SET(getPropertyTypeInfo()->qualifiers, EReflectTypeQualifiers::Constant))
        {
            const AsType *retVal = nullptr;
            const GlobalField<true, AsType> *memberFieldPtr = (const GlobalField<true, AsType> *)(propertyAccessor());

            fatalAssertf(memberFieldPtr, "Invalid Field pointer");
            retVal = &memberFieldPtr->get();
            return retVal;
        }
        else
        {
            AsType *retVal = nullptr;
            const GlobalField<false, AsType> *memberFieldPtr = (const GlobalField<false, AsType> *)(propertyAccessor());

            fatalAssertf(memberFieldPtr, "Invalid Field pointer");
            retVal = &memberFieldPtr->get();
            return retVal;
        }
    }
    template <typename FromType>
    bool setFromTypeUnsafe(FromType &&value) const
    {
        using MemberType = std::remove_cvref_t<FromType>;

        // If constant then we use const MemberDataPointer
        if (BIT_SET(getPropertyTypeInfo()->qualifiers, EReflectTypeQualifiers::Constant))
        {
            LOG_ERROR("MemberDataProperty", "Cannot set constant value");
        }
        else
        {
            const GlobalField<false, MemberType> *memberFieldPtr = (const GlobalField<false, MemberType> *)(propertyAccessor());

            fatalAssertf(memberFieldPtr, "Invalid member pointer");
            memberFieldPtr->set(std::forward<FromType>(value));
            return true;
        }
        return false;
    }
};

//////////////////////////////////////////////////////////////////////////
// Templated Implementations
//////////////////////////////////////////////////////////////////////////

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
    const void *propertyAccessor() const override { return &memberField; }
    /* MemberFieldWrapper overrides */
public:
    void *get(void *object) const override
    {
        if CONST_EXPR (std::is_const_v<MemberType>)
        {
            LOG_ERROR("MemberFieldWrapperImpl", "Use const object function to retrieve const value");
            return nullptr;
        }
        else
        {
            ObjectType *outerObject = (ObjectType *)(object);
            return &memberField.get(outerObject);
        }
    }

    const void *get(const void *object) const override
    {
        const ObjectType *outerObject = (const ObjectType *)(object);
        return &memberField.get(outerObject);
    }
    void setTypeless(void *value, void *object) const override
    {
        if CONST_EXPR (!std::is_const_v<MemberType>)
        {
            ObjectType *outerObject = (ObjectType *)(object);
            MemberType *valuePtr = (MemberType *)(value);
            memberField.set(outerObject, *valuePtr);
        }
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
    const void *propertyAccessor() const override { return &field; }
    /* GlobalFieldWrapper overrides */
public:
    FieldValuePtr<void> get() const override { return &field.get(); }
    void setTypeless(void *value) const override
    {
        if CONST_EXPR (!std::is_const_v<MemberType>)
        {
            MemberType *valuePtr = (MemberType *)(value);
            field.set(*valuePtr);
        }
    }

    /* Override ends */
};
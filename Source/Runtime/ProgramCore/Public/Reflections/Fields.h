/*!
 * \file Fields.h
 *
 * \author Jeslas Pravin
 * \date January 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, Since 2022
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "Types/CoreDefines.h"
#include "Types/CoreTypes.h"

#include <concepts>

// Array related concepts, CleanType is necessary as array related type traits works without any
// reference type Element wise assigner is array's element is not same or pointers
template <typename MemberType, typename Type, typename CleanType = std::remove_cvref_t<Type>>
concept ArrayElementAssignableFrom
    = std::is_array_v<MemberType>
      && ((std::is_array_v<CleanType>
           && std::negation_v<std::is_same<std::remove_all_extents_t<MemberType>, std::remove_all_extents_t<CleanType>>>
           && (sizeof(MemberType) <= sizeof(CleanType))
           && std::assignable_from<
               std::remove_all_extents_t<MemberType> &,
               std::remove_all_extents_t<CleanType>>) // Is element assignable
          || std::is_pointer_v<CleanType>
                 && std::assignable_from<std::remove_all_extents_t<MemberType> &, std::remove_pointer_t<CleanType>>); // Is
                                                                                                                      // pointer
                                                                                                                      // assignable
// Array direct assign by array, If array's element type is same
template <typename MemberType, typename Type, typename CleanType = std::remove_cvref_t<Type>>
concept ArrayAssignableFrom = std::is_array_v<MemberType> && std::is_array_v<CleanType>
                              && std::is_same_v<std::remove_all_extents_t<MemberType>, std::remove_all_extents_t<CleanType>>
                              && (sizeof(MemberType) <= sizeof(CleanType));
// Just element assign at an index
template <typename MemberType, typename Type>
concept ElementAssignableFrom = std::is_array_v<MemberType> && std::assignable_from<std::remove_all_extents_t<MemberType> &, Type>;

// Not array related concepts
template <typename MemberType, typename Type>
concept DirectAssignableFrom = std::negation_v<std::is_array<MemberType>> && std::assignable_from<MemberType &, Type>;

template <bool IsConst, typename MemberType>
class GlobalField;

template <typename MemberType>
class GlobalField<false, MemberType>
{
public:
    typedef MemberType(*GlobalFieldPtr);

private:
    GlobalFieldPtr globalFieldPtr;

public:
    GlobalField() = default;
    MAKE_TYPE_DEFAULT_COPY_MOVE(GlobalField)

    GlobalField(GlobalFieldPtr fieldPtr)
        : globalFieldPtr(fieldPtr)
    {}
    GlobalField &operator= (GlobalFieldPtr fieldPtr)
    {
        globalFieldPtr = fieldPtr;
        return *this;
    }

    operator bool () const { return globalFieldPtr != nullptr; }

    template <typename Type>
    requires DirectAssignableFrom<MemberType, Type>
    void set(Type &&newValue) const
    {
        *globalFieldPtr = std::forward<Type>(newValue);
    }

    // Assigns each element of an array or pointer to array of elements into this array
    // ** The assigning from array must be at least of length assigning to array else use individual
    // element wise assign
    template <typename Type>
    requires ArrayElementAssignableFrom<MemberType, Type>
    void set(Type &&newValue) const
    {
        // Find element type from pointer or array
        using AssignFromElementType = std::conditional_t<std::is_pointer_v<Type>, std::remove_pointer_t<Type>, std::remove_all_extents_t<Type>>;

        MemberType &memberValue = *globalFieldPtr;
        const uint32 arrayLen = uint32(ARRAY_LENGTH(memberValue));

        using ElementType = std::remove_all_extents_t<MemberType>;
        ElementType *memberValPtr = &memberValue;
        const AssignFromElementType *newValPtr = std::forward<Type>(newValue);
        for (uint32 i = 0; i < arrayLen; ++i)
        {
            memberValPtr[i] = newValPtr[i];
        }
    }

    template <typename Type>
    requires ArrayAssignableFrom<MemberType, Type>
    void set(Type &&newValue) const
    {
        memcpy(*globalFieldPtr, std::forward<Type>(newValue), sizeof(MemberType));
    }

    template <typename Type>
    requires ElementAssignableFrom<MemberType, Type>
    void set(Type &&newValue, uint32 index) const
    {
        (*globalFieldPtr)[index] = std::forward<Type>(newValue);
    }

    MemberType &get() const { return *globalFieldPtr; }
};

template <typename MemberType>
class GlobalField<true, MemberType>
{
public:
    typedef const MemberType(*GlobalFieldPtr);

private:
    GlobalFieldPtr globalFieldPtr;

public:
    GlobalField() = default;
    MAKE_TYPE_DEFAULT_COPY_MOVE(GlobalField)

    GlobalField(GlobalFieldPtr fieldPtr)
        : globalFieldPtr(fieldPtr)
    {}
    GlobalField &operator= (GlobalFieldPtr fieldPtr)
    {
        globalFieldPtr = fieldPtr;
        return *this;
    }

    operator bool () const { return globalFieldPtr != nullptr; }

    const MemberType &get() const { return *globalFieldPtr; }
};

//////////////////////////////////////////////////////////////////////////
// Class Member field
//////////////////////////////////////////////////////////////////////////

template <bool IsConst, typename ClassType, typename MemberType>
struct MemberFieldOffsetStorage
{
public:
    using DeclaredMemberType = std::conditional_t<IsConst, const MemberType, MemberType>;
    typedef DeclaredMemberType(ClassType::*DeclaredMemberFieldPtr);
    using MemberFieldPtr = DeclaredMemberFieldPtr;

    MemberFieldOffsetStorage() = default;
    MAKE_TYPE_DEFAULT_COPY_MOVE(MemberFieldOffsetStorage)

    MemberFieldOffsetStorage(MemberFieldPtr memberField)
        : offset((OffsetType) reinterpret_cast<PtrInt>(&(reinterpret_cast<ClassType *>(0)->*memberField)))
    {}
    MemberFieldOffsetStorage &operator= (MemberFieldPtr memberField)
    {
        offset = (OffsetType) reinterpret_cast<PtrInt>(&(reinterpret_cast<ClassType *>(0)->*memberField));
        return *this;
    }

    template <typename ObjectType>
    requires std::is_same_v<ClassType, std::remove_const_t<ObjectType>>
    FORCE_INLINE DeclaredMemberType &memberDeref(ObjectType *object) const
    {
        return *reinterpret_cast<DeclaredMemberType *>(reinterpret_cast<UPtrInt>(object) + offset);
    }
    FORCE_INLINE bool isValid() const { return offset >= 0; }

private:
    using OffsetType = int32;

    // Offset will always be positive
    OffsetType offset = -1;
};

template <bool IsConst, typename ClassType, typename MemberType>
struct MemberFieldPtrStorage
{
public:
    typedef MemberType(ClassType::*NonConstMemberFieldPtr);
    typedef const MemberType(ClassType::*ConstMemberFieldPtr);

    using MemberFieldPtr = std::conditional_t<IsConst, ConstMemberFieldPtr, NonConstMemberFieldPtr>;

    MemberFieldPtrStorage() = default;
    MAKE_TYPE_DEFAULT_COPY_MOVE(MemberFieldPtrStorage)

    MemberFieldPtrStorage(MemberFieldPtr memberField)
        : fieldPtr(memberField)
    {}
    MemberFieldPtrStorage &operator= (MemberFieldPtr memberField)
    {
        fieldPtr = memberField;
        return *this;
    }

    template <typename ObjectType>
    requires std::is_same_v<ClassType, std::remove_const_t<ObjectType>>
    FORCE_INLINE auto &memberDeref(ObjectType *object) const
    {
        return (object->*fieldPtr);
    }
    FORCE_INLINE bool isValid() const { return fieldPtr != nullptr; }

private:
    MemberFieldPtr fieldPtr;
};

// Can be both class and struct member
template <bool IsConst, typename ClassType, typename MemberType>
class ClassMemberField;

template <typename ClassType, typename MemberType>
class ClassMemberField<false, ClassType, MemberType>
{
public:
    // Replace with MemberFieldPtrStorage or MemberFieldOffsetStorage
    using MemberFieldStorage = MemberFieldOffsetStorage<false, ClassType, MemberType>;
    using ConstMemberType = const MemberType;
    using MemberFieldPtr = MemberFieldStorage::MemberFieldPtr;

private:
    MemberFieldStorage storage;

public:
    ClassMemberField() = default;
    MAKE_TYPE_DEFAULT_COPY_MOVE(ClassMemberField)

    ClassMemberField(MemberFieldPtr memberField)
        : storage(memberField)
    {}
    ClassMemberField &operator= (MemberFieldPtr fieldPtr)
    {
        storage = fieldPtr;
        return *this;
    }

    operator bool () const { return storage.isValid(); }

    template <typename Type>
    requires DirectAssignableFrom<MemberType, Type>
    void set(ClassType *object, Type &&newValue) const
    {
        storage.memberDeref(object) = std::forward<Type>(newValue);
    }
    template <typename Type>
    requires DirectAssignableFrom<MemberType, Type>
    void set(ClassType &object, Type &&newValue) const
    {
        set<Type>(&object, std::forward<Type>(newValue));
    }

    template <typename Type>
    requires ArrayElementAssignableFrom<MemberType, Type>
    void set(ClassType *object, Type &&newValue) const
    {
        // Find element type from pointer or array
        using AssignFromElementType = std::conditional_t<std::is_pointer_v<Type>, std::remove_pointer_t<Type>, std::remove_all_extents_t<Type>>;

        MemberType &memberValue = storage.memberDeref(object);
        const uint32 arrayLen = uint32(ARRAY_LENGTH(memberValue));

        using ElementType = std::remove_all_extents_t<MemberType>;
        ElementType *memberValPtr = &memberValue;
        const AssignFromElementType *newValPtr = std::forward<Type>(newValue);
        for (uint32 i = 0; i < arrayLen; ++i)
        {
            memberValPtr[i] = newValPtr[i];
        }
    }
    template <typename Type>
    requires ArrayElementAssignableFrom<MemberType, Type>
    void set(ClassType &object, Type &&newValue) const
    {
        set<Type>(&object, std::forward<Type>(newValue));
    }

    template <typename Type>
    requires ArrayAssignableFrom<MemberType, Type>
    void set(ClassType *object, Type &&newValue) const
    {
        memcpy(&storage.memberDeref(object), &newValue, sizeof(MemberType));
    }
    template <typename Type>
    requires ArrayAssignableFrom<MemberType, Type>
    void set(ClassType &object, Type &&newValue) const
    {
        set<Type>(&object, std::forward<Type>(newValue));
    }

    template <typename Type>
    requires ElementAssignableFrom<MemberType, Type>
    void set(ClassType *object, Type &&newValue, uint32 index) const
    {
        storage.memberDeref(object)[index] = std::forward<Type>(newValue);
    }
    template <typename Type>
    requires ElementAssignableFrom<MemberType, Type>
    void set(ClassType &object, Type &&newValue, uint32 index) const
    {
        set<Type>(&object, std::forward<Type>(newValue), index);
    }

    MemberType &get(ClassType &object) const { return storage.memberDeref(&object); }
    MemberType &get(ClassType *object) const { return storage.memberDeref(object); }

    ConstMemberType &get(const ClassType &object) const { return storage.memberDeref(&object); }
    ConstMemberType &get(const ClassType *object) const { return storage.memberDeref(object); }
};
template <typename ClassType, typename MemberType>
class ClassMemberField<true, ClassType, MemberType>
{
public:
    using MemberFieldStorage = MemberFieldOffsetStorage<true, ClassType, MemberType>;
    using ConstMemberType = const MemberType;
    using MemberFieldPtr = MemberFieldStorage::MemberFieldPtr;

private:
    MemberFieldStorage storage;

public:
    ClassMemberField() = default;
    MAKE_TYPE_DEFAULT_COPY_MOVE(ClassMemberField)

    ClassMemberField(MemberFieldPtr memberField)
        : storage(memberField)
    {}
    ClassMemberField &operator= (MemberFieldPtr fieldPtr)
    {
        storage = fieldPtr;
        return *this;
    }

    operator bool () const { return storage.isValid(); }

    ConstMemberType &get(const ClassType &object) const { return storage.memberDeref(&object); }
    ConstMemberType &get(const ClassType *object) const { return storage.memberDeref(object); }
};